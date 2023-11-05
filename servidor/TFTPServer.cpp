#include "TFTPServer.h"

using namespace std;

TFTPServer::TFTPServer(sockpp::UDPSocket & sock, sockpp::AddrInfo & addr, int timeout, string rootDirectory)
    : Callback(sock.get_descriptor(), timeout), sock(sock), addr(addr), request(0), data(0), ack(0), error(0), outputFile(0), rootDir(rootDirectory), estado(Estado::Espera), timeoutState(false) {
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
            cout << "Estado Espere" << endl;
            disable_timeout();
            // Recebe o pacote
            bytesAmount = sock.recv(buffer, sizeof(buffer), addr);
           
            if (bytesAmount > 0) {
                request = new Request(buffer, bytesAmount);

                if (request->getOpcode() == 1){
                    cout << "Recebeu um RRQ" << endl;
                    string filepath = rootDir + "/" + request->getFilename();

                    cout << "Tentando abrir o arquivo " << filepath << endl;
                    // Testa se o arquivo existe
                    if (access(filepath.c_str(), F_OK) == -1){
                        error = new ERROR(1);
                        cout << "Erro " << error->getErrorCode() << ": " << error->getErrorMessage() << endl;
                        sock.send(error->data(), error->size(), addr);
			return;

                    // Testa se o arquivo tem permissão de leitura
		    } else if (access(filepath.c_str(), R_OK) == -1){
                        error = new ERROR(2);
                        cout << "Erro " << error->getErrorCode() << ": " << error->getErrorMessage() << endl;
                        sock.send(error->data(), error->size(), addr);
                        return;
                    } else {
                        estado = Estado::Transmitir;
                        data = new DATA(filepath);
                        ack = new ACK();
                        cout << "Passando para o estado Transmitir" << endl;
                        sock.send((char*)data, data->size(), addr); // Enviar o pacote DATA
                        enable_timeout();
                        return;
                    }

                } else if (request->getOpcode() == 2){
                    cout << "Recebeu um WRQ" << endl;
                    string filepath = rootDir + "/" + request->getFilename();

                    cout << "Recebendo o arquivo \"" << filepath << "\"..." << endl;
                    if (access(filepath.c_str(), F_OK) == 0){
                        error = new ERROR(6);
                        cout << "Erro " << error->getErrorCode() << ": " << error->getErrorMessage() << endl;
                        sock.send(error->data(), error->size(), addr);
                        return;

                    } else {
                        estado = Estado::Receber;
                        ack = new ACK();
                        data = new DATA();
                        outputFile = new ofstream(filepath);
                        sock.send((char*)ack, sizeof(ACK), addr); // Enviar o pacote ACK para o cliente
                        enable_timeout();
                        return;
                    }

                } else if (request->getOpcode() > 2 && request->getOpcode() < 6){
                    cout << "Recebeu opcode " << request->getOpcode() << endl;
                    error = new ERROR(4);
                    cout << "Erro " << error->getErrorCode() << ": " << error->getErrorMessage() << endl;
                    sock.send(error->data(), error->size(), addr);
                    return;

                } else {
                    error = new ERROR(0);
                    cout << "Erro " << error->getErrorCode() << ": " << error->getErrorMessage() << endl;
                    sock.send(error->data(), error->size(), addr);
                    return;
                    
                }
	    } else {
		cout << "Erro ao receber pacote" << endl;
		return;
	    }
            break;

        case Estado::Transmitir:
            cout << "Estado Transmitir" << endl;

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
            cout << "Estado Receber" << endl;

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
            cout << "Estado Fim" << endl;
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

void TFTPServer::resetAll() {
    if (request != 0) delete request;
    if (data != 0) delete data;
    if (ack != 0) delete ack;
    if (error != 0) delete error;
    if (outputFile != 0) delete outputFile;
    estado = Estado::Espera;
    bytesAmount = 0;
    timeoutCounter = 0;
    timeoutState = false;
    disable_timeout();
}
