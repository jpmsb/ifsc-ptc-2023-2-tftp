#include "TFTP.h"

using namespace std;

TFTP::TFTP(sockpp::UDPSocket & sock, sockpp::AddrInfo & addr, int timeout, string & operacao, string & sourceFile, string & destinationFile)
    : Callback(sock.get_descriptor(), timeout), sock(sock), addr(addr), operation(operacao), srcFile(sourceFile), destFile(destinationFile) {
    disable_timeout();
    cout << "Início!!" << endl;
    if (operation == "enviar"){
        wrq = new WRQ(destFile);
    } else if (operation == "receber"){
        rrq = new RRQ(srcFile);
    }
    estado = Estado::Conexao;
    start(); // inicializa primeiro pacote
  }

void TFTP::handle() {
    switch (estado) {
        case Estado::Conexao:
            cout << "Conexão!! " << endl;
            if (operation == "enviar"){
                cout << "Enviando o arquivo \"" << srcFile << "\"!!" << endl;
                sock.send(wrq->data(), wrq->size(), addr);
                data = new DATA(srcFile);

                // Preparando para receber ACK
                sock.recv(buffer, 4, addr);
                
                ack = new ACK();
                ack->setBytes(buffer); // Salvar bytes do ACK no objeto ack

                if (ack->getOpcode() == 4) {
                    cout << "(Conexão) Recebeu ACK " << ack->getBlock() << "!" << endl;

                    cout << "Transmitindo \"" << data->dataSize() << "\" bytes..." << endl;
                    sock.send((char*)data, data->size(), addr);
                    estado = Estado::Transmitir;

                } else if (data->getOpcode() == 5) {
                   cout << "Recebeu um pacote de ERRO!" << endl;
                   error = new ERROR(buffer, bytesAmount);

                   cout << "Erro " << error->getErrorCode() << ": " << error->getErrorMessage() << endl;
                   estado = Estado::Fim;
                   start();

                } else {
                   estado = Estado::Fim;
                   start();
                };


            } else if (operation == "receber"){
                cout << "Recebendo o arquivo \"" << srcFile << "\"!!" << endl;
                sock.send(rrq->data(), rrq->size(), addr);

                // Preparando para receber DATA
                bytesAmount = sock.recv(buffer, 516, addr);
                data = new DATA(buffer, bytesAmount); // Salvar bytes do DATA no objeto data

                if (data->getOpcode() == 3){
                   cout << "(Conexão) Recebeu DATA " << data->getBlock() << "!" << endl;
                   outputFile = new ofstream(destFile);

                   // Escrever os dados recebidos no arquivo
                   outputFile->write(data->getData(), data->dataSize());

                   // Mandar um ACK para o servidor
                   ack = new ACK();
                   ack->increment();
                   sock.send((char*)ack, sizeof(ACK), addr);                     

                   estado = Estado::Receber;
                   
                } else if (data->getOpcode() == 5) {
                   cout << "Recebeu um pacote de ERRO!" << endl;
                   error = new ERROR(buffer, bytesAmount);

                   cout << "Erro " << error->getErrorCode() << ": " << error->getErrorMessage() << endl;
                   estado = Estado::Fim;
                   start();

                } else {
                   estado = Estado::Fim;
                   start();
                };
                
            }
            break;

        case Estado::Transmitir:
            sock.recv(buffer, 4, addr);

            if (ack->getOpcode() == 4) {
                ack->setBytes(buffer);
                cout << "Recebeu ACK " << ack->getBlock() << "!" << endl;
                data->increment();

            } else if (data->getOpcode() == 5) {
               cout << "Recebeu um pacote de ERRO!" << endl;
               error = new ERROR(buffer, bytesAmount);

               cout << "Erro " << error->getErrorCode() << ": " << error->getErrorMessage() << endl;
               estado = Estado::Fim;
               start();
           
            } else {
               estado = Estado::Fim;
               start();
            }

            cout << "Transmitindo \"" << data->dataSize() << "\" bytes..." << endl;
            if (data->dataSize() >= 512) {
                sock.send((char*)data, data->size(), addr);
            } else { // Último pacote
                sock.send((char*)data, data->size(), addr);
                estado = Estado::Fim;
            }

            break;
        
        case Estado::Receber:
            // Preparando para receber DATA
            bytesAmount = sock.recv(buffer, 516, addr);
            data = new DATA(buffer, bytesAmount); // Salvar bytes do DATA no objeto data
 
            if (data->getOpcode() == 3){ 
               cout << "(Recv) Recebeu DATA " << data->getBlock() << "!" << endl;

               // Escrever os dados recebidos no arquivo
               outputFile->write(data->getData(), data->dataSize());

               // Mandar um ACK para o servidor
               cout << "Enviando ACK" << endl;
               ack->increment();
               sock.send((char*)ack, sizeof(ACK), addr);                     

               if (data->dataSize() < 512) {
                   outputFile->close();
                   estado = Estado::Fim;
                   start();
               }
  
            } else if (data->getOpcode() == 5) {
               cout << "Recebeu um pacote de ERRO!" << endl;
               error = new ERROR(buffer, bytesAmount);
               cout << "Erro " << error->getErrorCode() << ": " << error->getErrorMessage() << endl;
               estado = Estado::Fim;
               start();

            } else {
               estado = Estado::Fim;
               start();
            }
            break;
        
        case Estado::Fim:
            cout << "Fim!" << endl;
            exit(0);
            break;
    }
}

void TFTP::handle_timeout() {
}

void TFTP::start() {
    handle();
}

