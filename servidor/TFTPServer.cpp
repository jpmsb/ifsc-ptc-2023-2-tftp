#include "TFTPServer.h"

using namespace std;

TFTPServer::TFTPServer(sockpp::UDPSocket & sock, sockpp::AddrInfo & addr, int timeout, string rootDirectory)
    : Callback(sock.get_descriptor(), timeout), sock(sock), addr(addr), request(0), data(0), ack(0), error(0), outputFile(0), desserializedMessage(0), rootDir(rootDirectory), estado(Estado::Espera), timeoutState(false) {
    // Por garantia, desabilita o timeout
    disable_timeout();

    start(); // dispara a máquina de estados
  }

TFTPServer::~TFTPServer(){
    resetAll();
}

void TFTPServer::handle() {
    uint16_t ackBlock = 0;
    switch (estado) {
        case Estado::Espera:
            clearAll();
            disable_timeout();

            // Recebe o pacote
            bytesAmount = sock.recv(buffer, sizeof(buffer), addr);
           
            if (bytesAmount > 0) {
                request = new Request(buffer, bytesAmount);
                string serializedMessage(buffer);
                desserializedMessage = new tftp2::Mensagem();
                desserializedMessage->ParseFromString(serializedMessage);

                if (desserializedMessage->has_list()){
                    string fullPath = rootDir + "/" + desserializedMessage->list().path();
                    cout << "Listando o conteúdo do diretório \"" << fullPath << "\"\n";

                    tftp2::ListResponse listResponse;
                    int listResult = listDirectoryContents(fullPath, &listResponse);

                    if (listResult == 0) {
                        tftp2::Mensagem message;
                        message.mutable_list_response()->mutable_items()->CopyFrom(listResponse.items());
                        
                        estado = Estado::Transmitir;
                        data = new DATA(message);
                        ack = new ACK();
                        sock.send((char*)data, data->size(), addr); // Enviar o pacote DATA
                        enable_timeout();
                       
                    } else {
                        error = createErrorFromSysCallError(listResult);
                        cout << "Erro " << error->getErrorCode() << ": " << error->getErrorMessage() << endl;
                        sock.send(error->data(), error->size(), addr);
                    }  


                } else if (desserializedMessage->has_move()){
                    string oldName = desserializedMessage->move().old_name();
                    string newName = desserializedMessage->move().new_name();

                    if (newName.size() > 0) cout << "Renomeando \"" << oldName << "\" para \"" << newName << "\"" << endl;
                    else cout << "Removendo \"" << oldName << "\"" << endl;

                    int moveResult = moveElement(oldName, newName);
                    if (moveResult == 0) {
                        ack = new ACK();
                        sock.send((char*)ack, sizeof(ACK), addr); // Enviar o pacote ACK para o cliente

                    } else {
                        error = createErrorFromSysCallError(moveResult);
                        cout << "Erro " << error->getErrorCode() << ": " << error->getErrorMessage() << endl;
                        sock.send(error->data(), error->size(), addr);
                    }

                } else if (desserializedMessage->has_mkdir()){
                    cout << "Criando diretório: " << desserializedMessage->mkdir().path() << endl;
                    string fullPath = rootDir + "/" + desserializedMessage->mkdir().path();

                    int mkdirResult = createDirectory(fullPath);

                    if (mkdirResult == 0) {
                        ack = new ACK();
                        sock.send((char*)ack, sizeof(ACK), addr); // Enviar o pacote ACK para o cliente

                    } else {
                        error = createErrorFromSysCallError(mkdirResult);
                        cout << "Erro " << error->getErrorCode() << ": " << error->getErrorMessage() << endl;
                        sock.send(error->data(), error->size(), addr);
                    }

                } else if (request->getOpcode() == 1){
                    cout << "Recebeu um RRQ" << endl;
                    string filepath = rootDir + "/" + request->getFilename();

                    cout << "Tentando abrir o arquivo " << filepath << endl;
                    // Testa se o arquivo existe
                    if (access(filepath.c_str(), F_OK) == -1){
                        error = new ERROR(1);
                        cout << "Erro " << error->getErrorCode() << ": " << error->getErrorMessage() << endl;
                        sock.send(error->data(), error->size(), addr);

                    // Testa se o arquivo tem permissão de leitura
                    } else if (access(filepath.c_str(), R_OK) == -1){
                        error = new ERROR(2);
                        cout << "Erro " << error->getErrorCode() << ": " << error->getErrorMessage() << endl;
                        sock.send(error->data(), error->size(), addr);
                    } else {
                        estado = Estado::Transmitir;
                        data = new DATA(filepath);
                        ack = new ACK();
                        sock.send((char*)data, data->size(), addr); // Enviar o pacote DATA
                        enable_timeout();
                    }

                } else if (request->getOpcode() == 2){
                    cout << "Recebeu um WRQ" << endl;
                    string filepath = rootDir + "/" + request->getFilename();

                    cout << "Recebendo o arquivo \"" << filepath << "\"..." << endl;
                    if (access(filepath.c_str(), F_OK) == 0){
                        error = new ERROR(6);
                        cout << "Erro " << error->getErrorCode() << ": " << error->getErrorMessage() << endl;
                        sock.send(error->data(), error->size(), addr);

                    } else {
                        estado = Estado::Receber;
                        ack = new ACK();
                        data = new DATA();
                        outputFile = new ofstream(filepath);
                        sock.send((char*)ack, sizeof(ACK), addr); // Enviar o pacote ACK para o cliente
                        enable_timeout();
                    }

                } else if (request->getOpcode() > 2 && request->getOpcode() < 6){
                    error = new ERROR(4);
                    cout << "Erro " << error->getErrorCode() << ": " << error->getErrorMessage() << endl;
                    sock.send(error->data(), error->size(), addr);

                } else {
                    error = new ERROR(0);
                    cout << "Erro " << error->getErrorCode() << ": " << error->getErrorMessage() << endl;
                    sock.send(error->data(), error->size(), addr);
                    
                }
	    } else {
                // Caso receba um pacote nulo
                error = new ERROR(0);
                cout << "Erro " << error->getErrorCode() << ": " << error->getErrorMessage() << endl;
                sock.send(error->data(), error->size(), addr);
	    }
            return;
            break;

        case Estado::Transmitir:
            if (timeoutState) {
		cout << "Reenviando pacote DATA..." << endl;
                sock.send((char*)data, data->size(), addr); // Enviar o pacote DATA
		timeoutState = false;
                reload_timeout();
                return;
	    }

            sock.recv(buffer, 4, addr); // Receber o pacote ACK
            ack->setBytes(buffer);

            if (ack->getOpcode() == 4 && ack->getBlock() == data->getBlock()){ // Verificando se o pacote recebido é o certo
                cout << "Recebeu pacote ACK " << ack->getBlock() << endl;
                if (data->dataSize() < 512){ // Último pacote data
                    cout << "Último pacote ACK" << endl;
                    estado = Estado::Espera;
                    disable_timeout();
                    return;
                } else {
                   data->increment();
                   sock.send((char*)data, data->size(), addr); // Enviar o pacote DATA
                   reload_timeout();
                }
                return;
            } else {
                error = new ERROR(4);
                cout << "Erro " << error->getErrorCode() << ": " << error->getErrorMessage() << endl;
                sock.send(error->data(), error->size(), addr);
                estado = Estado::Espera;
                disable_timeout();
                return;
            }
            break;
        
        case Estado::Receber:
            if (timeoutState) {
	        cout << "Reenviando pacote ACK..." << endl;
                sock.send((char*)ack, sizeof(ACK), addr); // Enviar o pacote ACK para o cliente
	        timeoutState = false;
                reload_timeout();
                return;
	    }

            bytesAmount = sock.recv(buffer, sizeof(buffer), addr); // Receber o pacote DATA
            data->setBytes(buffer, bytesAmount);
            ackBlock = data->getBlock() - 1;

            if (data->getOpcode() == 3 && ackBlock == ack->getBlock()){ // Verificando se o pacote recebido é o certo
		cout << "Recebeu pacote DATA " << data->getBlock() << ", Tamanho: " << data->dataSize() << endl;
		outputFile->write(data->getData(), data->dataSize());
		ack->increment();
                sock.send((char*)ack, sizeof(ACK), addr); // Enviar o pacote ACK para o cliente
                if (data->dataSize() < 512){ // Último pacote data
                    outputFile->close(); // O arquivo é sincronizado no armazenamento
                    estado = Estado::Espera;
                    disable_timeout();
                    return;
	        }

	    } else {
		error = new ERROR(4);
		cout << "Erro " << error->getErrorCode() << ": " << error->getErrorMessage() << endl;
		sock.send(error->data(), error->size(), addr);
		estado = Estado::Espera;
                disable_timeout();
		return;
	    }
            
            break;

        case Estado::Fim:
            estado = Estado::Espera;
            return;
            break;
    }
}

void TFTPServer::handle_timeout() {
    cout << "Timeout: " << (int)timeoutCounter << endl;
    timeoutState = true;
    if (timeoutCounter == 3) {
        cout << "Timeout: voltando para o estado de espera." << endl;
        resetAll();
	return;
    } else {
        timeoutCounter++;
        reload_timeout();
        start();
    }
}

void TFTPServer::start() {
    handle();
}

void TFTPServer::clearAll() {
    if (request != 0) 
        delete request;
        request = 0;

    if (data != 0) 
        delete data;
        data = 0;

    if (ack != 0)
        delete ack;
        ack = 0;

    if (error != 0)
        delete error;
        error = 0;

    if (outputFile != 0)
        delete outputFile;
        outputFile = 0;

    if (desserializedMessage != 0)
        delete desserializedMessage;
        desserializedMessage = 0;

    memset(buffer, 0, sizeof(buffer));
    bytesAmount = 0;
    timeoutCounter = 0;
    timeoutState = false;
}

void TFTPServer::resetAll() {
    clearAll();
    estado = Estado::Espera;
}

int TFTPServer::listDirectoryContents(string path, tftp2::ListResponse * listResponse){
    DIR *dir = opendir(path.c_str());
    struct dirent *entry;
    struct stat fileStat;

    if (dir != 0){
        while ((entry = readdir(dir)) != 0){
	    string itemName(entry->d_name);
	    string itemPath = path + "/" + itemName;

            if (itemName == "." || itemName == "..") continue;

            tftp2::ListItem* item = listResponse->add_items();

	    if (stat(itemPath.c_str(), &fileStat) == 0 && (fileStat.st_mode & S_IFDIR)){
                item->mutable_directory()->set_path(itemName);
	    } else {
                item->mutable_file()->set_name(itemName);
                item->mutable_file()->set_size(fileStat.st_size);
	    }
	}
    } else return errno;
    return 0; 
}

int TFTPServer::createDirectory(string path) {
    if (mkdir(path.c_str(), 0700) == -1){
	return errno;
    }
    return 0;
}

int TFTPServer::moveElement(string oldName, string newName) {
    string oldPath = rootDir + "/" + oldName;

    if (newName.size() > 0){
        string newPath = rootDir + "/" + newName;
        if (rename(oldPath.c_str(), newPath.c_str()) == -1){
            return errno;
        }
    } else {
	if (remove(oldPath.c_str()) == -1){
	    return errno;
	}
    }
    return 0;
}

ERROR* TFTPServer::createErrorFromSysCallError(int errorNumber) {
    ERROR * error;

    if (errorNumber == EACCES || errorNumber == EFAULT || errorNumber == EROFS) error = new ERROR(2);
    else if (errorNumber == EDQUOT || errorNumber == ENOSPC) error = new ERROR(3);
    else if (errorNumber == EEXIST) error = new ERROR(6);
    else if (errorNumber == ENOENT) error = new ERROR(8);
    else if (errorNumber == ENOTEMPTY) error = new ERROR(9);
    else if (errorNumber == ENOTDIR) error = new ERROR(10);
    else error = new ERROR(0);

    return error;
}
