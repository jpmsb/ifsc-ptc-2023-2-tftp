#include "TFTP.h"
#include <iostream>

using namespace std;

// Construtor da classe TFTP, que recebe como parâmetros:
// sock: referência para socktet UDP, sendo um objeto do tipo UDPSocket, do namespace sockpp
// addr: referência para endereçamento de erro, sendo um objeto do tipo AddrInfo, do namespace sockpp
// timeout: um inteiro para definir o tempo limite de espera por uma resposta do servidor
// operation: enumeração que define se o cliente solicitará envio ou recebimento de arquivo
// sourceFile: referência para uma string que contém o nome do arquivo de origem
// destinationFile: referência para uma string que contém o nome do arquivo de destino
TFTP::TFTP(sockpp::UDPSocket & sock, sockpp::AddrInfo & addr, int timeout, Operation operation, string & sourceFile, string & destinationFile)
    : Callback(sock.get_descriptor(), timeout), sock(sock), addr(addr), operation(operation), srcFile(sourceFile), destFile(destinationFile), wrq(0), rrq(0), data(0), ack(0), error(0), outputFile(0), timeoutState(false), timeoutCounter(0), estado(Estado::Conexao), pbMessage(0) {
    // Por garantia, desabilita o timeout
    disable_timeout();
    start(); // inicializa primeiro pacote
  }

TFTP::~TFTP(){
    if (wrq != 0) delete wrq;
    if (rrq != 0) delete rrq;
    if (data != 0) delete data;
    if (ack != 0) delete ack;
    if (error != 0) delete error;
    if (outputFile != 0) delete outputFile;
    if (pbMessage != 0) delete pbMessage;
}

void TFTP::handle() {
    switch (estado) {
        case Estado::Conexao:
            // Rotina inicial da operação para envio de arquivo
            if (operation == Operation::SEND){
                if (timeoutState) {
		    timeoutState = false;
		    sock.send(wrq->data(), wrq->size(), addr);
		    reload_timeout();
		    return;
		}

                // Inicia a comunicação, enviando um pacote WRQ
                wrq = new WRQ(destFile);
                sock.send(wrq->data(), wrq->size(), addr);
                enable_timeout();
                estado = Estado::Transmitir;
                ack = new ACK();
                data = new DATA(srcFile);

            // Rotina inicial da operação para recebimento de arquivo
            } else if (operation == Operation::RECEIVE){
                if (timeoutState) {
                    timeoutState = false;
                    sock.send(rrq->data(), rrq->size(), addr);
                    reload_timeout();
                    return;
                }

                // Inicia a comunicação, enviando um pacote RRQ
                rrq = new RRQ(srcFile);
                sock.send(rrq->data(), rrq->size(), addr);
                enable_timeout();
                estado = Estado::Receber;
                ack = new ACK();
                
            } else if (operation == Operation::LIST){
                if (timeoutState) {
		    timeoutState = false;
                    sock.send(serializedMessage.c_str(), serializedMessage.size(), addr);
		    reload_timeout();
		    return;
		}

                pbMessage = new tftp2::Mensagem();
                tftp2::PATH* listMessage = pbMessage->mutable_list();
                listMessage->set_path(srcFile);
 
                string serializedMessage = pbMessage->SerializeAsString();
                sock.send(serializedMessage.c_str(), serializedMessage.size(), addr);
                enable_timeout();
                estado = Estado::Receber;
                ack = new ACK();
                return;

            } else if (operation == Operation::MOVE){
                if (timeoutState) {
                    timeoutState = false;
                    sock.send(serializedMessage.c_str(), serializedMessage.size(), addr);
                    reload_timeout();
                    return;
                }

                pbMessage = new tftp2::Mensagem();
                tftp2::MOVE* moveMessage = pbMessage->mutable_move();
                moveMessage->set_old_name(srcFile);
                moveMessage->set_new_name(destFile);
                
                serializedMessage = pbMessage->SerializeAsString();
                sock.send(serializedMessage.c_str(), serializedMessage.size(), addr);
                enable_timeout();
                estado = Estado::Resultado;

            } else if (operation == Operation::MKDIR){
                pbMessage = new tftp2::Mensagem();
                tftp2::PATH* mkdirMessage = pbMessage->mutable_mkdir();
                mkdirMessage->set_path(srcFile);
               
                string serializedMessage = pbMessage->SerializeAsString();
                sock.send(serializedMessage.c_str(), serializedMessage.size(), addr);
                enable_timeout();
                estado = Estado::Resultado;
            }
            break;

        case Estado::Transmitir:
            if (timeoutState) {
		timeoutState = false;
		sock.send((char*)data, data->size(), addr);
		reload_timeout();
                return;
	    }

            // Recebimento do possível ACK do servidor após o envio do
            // primeiro pacote DATA
            sock.recv(buffer, sizeof(buffer), addr);
            ack->setBytes(buffer); // Atribui os bytes recebidos no objeto ack

            // Verifica se o pacote recebido realmente é um ACK
            if (ack->getOpcode() == 4) {
                // Se os dados recebidos possuírem 512 bytes, continue a transmissão
                if (data->dataSize() >= 512) {
                    sock.send((char*)data, data->size(), addr); // Enviar o pacote DATA
                } else { // Último pacote
                    sock.send((char*)data, data->size(), addr); // Enviar o último DATA
                    estado = Estado::Fim; // Mudar o estado para o fim
                    return;
                }
                data->increment(); // Incrementar o número de bloco

            } else if (data->getOpcode() == 5) { // Verifica se recebeu um pacote de erro
               // Instanciação do pacote ERROR, já atribuindo os bystes recebidos anteriormente
               error = new ERROR(buffer, bytesAmount);

               // Dado o erro, o próximo estado é o fim.
               estado = Estado::Fim;
               throw error;

               // Como não receberá mais pacotes do servidor,
               // para que a máquina de estados finalize, é
               // preciso chamar a mesma mais uma vez
               finish();
               return;
           
            } else {                  // Se acontecer qualquer outra coisa
               estado = Estado::Fim;  // além do esperado, vá para o estado Fim
               finish();
               return;
            }

            break;
        
        case Estado::Receber:
            if (timeoutState) {
                timeoutState = false;
                sock.send((char*)ack, sizeof(ACK), addr);
                reload_timeout();
                return;
            }

            // Recebimento do possível DATA do servidor
            // // bytesAmount armazena a quantidade de bytes recebidos
            bytesAmount = sock.recv(buffer, 516, addr);
            data = new DATA(buffer, bytesAmount); // Salvar bytes do DATA no objeto data

            // Verifica se o pacote recebido realmente é um DATA
            if (data->getOpcode() == 3){
                if (operation == Operation::LIST) {
                    listingOutput.append(data->getData(), data->dataSize());
                } else {
                    if (outputFile == 0) outputFile = new ofstream(destFile);
                    // Escrever os dados recebidos no arquivo
                    outputFile->write(data->getData(), data->dataSize());
                }

               // Mandar um ACK para o servidor
               ack->increment(); // Incrementar o número de bloco do ack
               sock.send((char*)ack, sizeof(ACK), addr); // Enviar o ACK

               // Se o trecho do arquivo recebido for menor do que 512 bytes
               // o estado é definido para o Fim
               if (data->dataSize() < 512) {
                   if (operation == Operation::LIST) {
                       estado = Estado::Fim;
                       finish();
                       throw listingOutput;
                       return;
                   } else {
                       outputFile->close(); // O arquivo é sincronizado no armazenamento
                       estado = Estado::Fim;  // o estado é definido para o Fim
                       finish();
                       return;
                   }
               }
  
            } else if (data->getOpcode() == 5) { // Verifica se recebeu um pacote de erro
               // Instanciação do pacote ERROR, já atribuindo os bystes recebidos anteriormente
               error = new ERROR(buffer, bytesAmount);

               // Dado o erro, o próximo estado é o fim.
               estado = Estado::Fim;
               throw error;
               // finish();
               return; // Retorna para o poller

            } else { // Para qualquer outro evento o estado é o fim
               estado = Estado::Fim;
               finish();
               return;
            }
            break;

        case Estado::Resultado:
            bytesAmount = sock.recv(buffer, 516, addr);
            disable_timeout();

            ack = new ACK(buffer);

            if (ack->getOpcode() == 4) {
                estado = Estado::Fim;
                finish();
                return;
            } else if (ack->getOpcode() == 5) {
                error = new ERROR(buffer, bytesAmount);
                estado = Estado::Fim;
                throw error;
                finish();
                return;
            }
            break;
        
        case Estado::Fim:
            // Finaliza a rotina do poller
            finish();
            break;
    }
}

void TFTP::handle_timeout() {
    cout << "Timeout: " << (int)timeoutCounter << endl;
    timeoutState = true;
    if (timeoutCounter == 3) {
        estado = Estado::Fim;
        string error = "Timeout!";
        throw error;
        finish();
    } else {
        timeoutCounter++;
        reload_timeout();
        start();
    }
}

void TFTP::start() {
    handle();
}
