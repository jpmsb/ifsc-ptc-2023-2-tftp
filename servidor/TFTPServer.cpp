#include "TFTPServer.h"

using namespace std;

TFTPServer::TFTPServer(sockpp::UDPSocket & sock, sockpp::AddrInfo & addr, int timeout, string rootDirectory)
    : Callback(sock.get_descriptor(), timeout), sock(sock), addr(addr), request(0), data(0), ack(0), error(0), outputFile(0), rootDir(rootDirectory), estado(Estado::Espera) {
    // Por garantia, desabilita o timeout
    disable_timeout();

    start(); // dispara a máquina de estados
  }

TFTPServer::~TFTPServer(){
    if (request != 0) delete request;
    if (data != 0) delete data;
    if (ack != 0) delete ack;
    if (error != 0) delete error;
    if (outputFile != 0) delete outputFile;
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
			estado = Estado::Fim;
			start();

                    // Testa se o arquivo tem permissão de leitura
		    } else if (access(filepath.c_str(), R_OK) == -1){
                        error = new ERROR(2);
                        cout << "Erro " << error->getErrorCode() << ": " << error->getErrorMessage() << endl;
                        sock.send(error->data(), error->size(), addr);
                        estado = Estado::Fim;
                        start();
                    } else {
                        estado = Estado::Transmitir;
                        data = new DATA(filepath);
                        ack = new ACK();
                        enable_timeout();
                        start();
                    }

                } else if (request->getOpcode() == 2){
                    cout << "Recebeu um WRQ" << endl;
                    string filepath = rootDir + "/" + request->getFilename();

                    cout << "Recebendo o arquivo \"" << filepath << "\"..." << endl;
                    if (access(filepath.c_str(), F_OK) == 0){
                        error = new ERROR(6);
                        cout << "Erro " << error->getErrorCode() << ": " << error->getErrorMessage() << endl;
                        sock.send(error->data(), error->size(), addr);
                        estado = Estado::Fim;
                        start();

                    } else {
                        estado = Estado::Receber;
                        ack = new ACK();
                        data = new DATA();
                        outputFile = new ofstream(filepath);
                        enable_timeout();
                        start();
                    }

                } else if (request->getOpcode() > 2 && request->getOpcode() < 6){
                    cout << "Recebeu opcode " << request->getOpcode() << endl;
                    error = new ERROR(4);
                    cout << "Erro " << error->getErrorCode() << ": " << error->getErrorMessage() << endl;
                    sock.send(error->data(), error->size(), addr);
                    estado = Estado::Fim;
                    start();

                } else {
                    error = new ERROR(0);
                    cout << "Erro " << error->getErrorCode() << ": " << error->getErrorMessage() << endl;
                    sock.send(error->data(), error->size(), addr);
                    estado = Estado::Fim;
                    start();
                    
                }
	    } else {
		cout << "Erro ao receber pacote" << endl;
		estado = Estado::Fim;
	    }
            break;

        case Estado::Transmitir:
            cout << "Estado Transmitir" << endl;

            if (data->dataSize() < 512){ // Último pacote data
                estado = Estado::Fim;
            }

            sock.send((char*)data, data->size(), addr); // Enviar o pacote DATA
            sock.recv(buffer, 4, addr); // Receber o pacote ACK

            ack->setBytes(buffer);

            if (ack->getOpcode() == 4 && ack->getBlock() == data->getBlock()){ // Verificando se o pacote recebido é o certo
                cout << "Recebeu pacote ACK " << ack->getBlock() << endl;
                data->increment();
            } else {
                error = new ERROR(4);
                cout << "Erro " << error->getErrorCode() << ": " << error->getErrorMessage() << endl;
                sock.send(error->data(), error->size(), addr);
                estado = Estado::Fim;
                start();
            }

            break;
        
        case Estado::Receber:
            cout << "Estado Receber" << endl;

            sock.send((char*)ack, sizeof(ACK), addr); // Enviar o pacote ACK para o cliente
            bytesAmount = sock.recv(buffer, sizeof(buffer), addr); // Receber o pacote DATA

            data->setBytes(buffer, bytesAmount);
            ackBlock = data->getBlock() - 1;

            if (data->getOpcode() == 3 && ackBlock == ack->getBlock()){ // Verificando se o pacote recebido é o certo
		cout << "Recebeu pacote DATA " << data->getBlock() << endl;
		outputFile->write(data->getData(), data->dataSize());
		ack->increment();
                if (data->dataSize() < 512){ // Último pacote data
                    sock.send((char*)ack, sizeof(ACK), addr); // Enviar o pacote ACK para o cliente
                    outputFile->close(); // O arquivo é sincronizado no armazenamento
	            estado = Estado::Fim;
	        }

	    } else {
		error = new ERROR(4);
		cout << "Erro " << error->getErrorCode() << ": " << error->getErrorMessage() << endl;
		sock.send(error->data(), error->size(), addr);
		estado = Estado::Fim;
		start();
	    }
            
            break;
        
        case Estado::Fim:
            estado = Estado::Espera;
            start();
            break;
    }
}

void TFTPServer::handle_timeout() {
    start();
}

void TFTPServer::start() {
    handle();
}
