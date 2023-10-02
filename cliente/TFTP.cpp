#include "TFTP.h"

using namespace std;

TFTP::TFTP(sockpp::UDPSocket & sock, sockpp::AddrInfo & addr, int timeout, Operation operation, string & sourceFile, string & destinationFile)
    : Callback(sock.get_descriptor(), timeout), sock(sock), addr(addr), operation(operation), srcFile(sourceFile), destFile(destinationFile), transferStatus(false), wrq(0), rrq(0), data(0), ack(0), error(0), outputFile(0) {
    // Por garantia, desabilita o timeout
    disable_timeout();

    // Os pacotes que iniciam a comunicação com o servidor
    // são criados baseados no tipo de operação, ou seja,
    // são instanciados apenas se forem usados.
    if (operation == Operation::SEND){
        // Instanciação do pacote WRQ que salvará no
        // servidor o arquivo com o nome em "destFile"
        wrq = new WRQ(destFile);

    } else if (operation == Operation::RECEIVE){
        // Instanciação do pacote RRQ que está solicitando
        // o arquivo com o nome salvo em "srcFile"
        rrq = new RRQ(srcFile);
    }

    // Passa para o primeiro estado
    estado = Estado::Conexao;
    start(); // inicializa primeiro pacote
  }

TFTP::~TFTP(){
    if (wrq != 0) delete wrq;
    if (rrq != 0) delete rrq;
    if (data != 0) delete data;
    if (ack != 0) delete ack;
    if (error != 0) delete error;
    if (outputFile != 0) delete outputFile;
}

void TFTP::handle() {
    switch (estado) {
        case Estado::Conexao:
            // Rotina inicial da operação para envio de arquivo
            if (operation == Operation::SEND){
                // Inicia a comunicação, enviando um pacote WRQ
                sock.send(wrq->data(), wrq->size(), addr);

                // Esperando a resposta do servidor e guardando
                // a mesma em um arranjo de char chamado buffer
                sock.recv(buffer, 4, addr);
               
                // Instanciação do pacote ACK
                ack = new ACK();
                ack->setBytes(buffer); // Salvar os bytes recebidos no objeto ack

                // Verifica se o pacote recebido realmente é um ACK
                if (ack->getOpcode() == 4) {
                    // Instanciação do pacote DATA
                    data = new DATA(srcFile);

                    // Envia os primeiros 512 bytes do arquivo "srcFile"
                    sock.send((char*)data, data->size(), addr);

                    // Muda o estado para Transmitir
                    estado = Estado::Transmitir;

                } else if (data->getOpcode() == 5) { // Verifica se recebeu um pacote de erro
                   // Instanciação do pacote ERROR, já atribuindo os bystes recebidos anteriormente
                   error = new ERROR(buffer, bytesAmount);
 
                   // Dado o erro, o próximo estado é o fim.
                   estado = Estado::Fim;

                   // Como não receberá mais pacotes do servidor,
                   // para que a máquina de estados finalize, é
                   // preciso chamar a mesma mais uma vez
                   start();

                } else {                    // Se acontece qualquer outra coisa
                   estado = Estado::Fim;    // além do esperado, vá para o estado Fim
                   start();  // Chama a máquna de estados uma última vez
                };


            // Rotina inicial da operação para recebimento de arquivo
            } else if (operation == Operation::RECEIVE){
                // Inicia a comunicação, enviando um pacote RRQ
                sock.send(rrq->data(), rrq->size(), addr);

                // Esperando a resposta do servidor e guardando
                // a mesma em um arranjo de char chamado buffer
                // bytesAmount armazena a quantidade de bytes recebidos
                bytesAmount = sock.recv(buffer, 516, addr);
                data = new DATA(buffer, bytesAmount); // Salvar bytes do DATA no objeto data

                // Verifica se o pacote recebido realmente é um DATA
                if (data->getOpcode() == 3){
                   // Cria o arquivo onde os dados serão gravados.
                   // O nome do arquivo está em destFile
                   outputFile = new ofstream(destFile);

                   // Escrever os dados recebidos no arquivo
                   outputFile->write(data->getData(), data->dataSize());

                   // Instanciação do pacote ACK
                   ack = new ACK();
                   ack->increment(); // Incrementar o número de bloco do ACK
                   sock.send((char*)ack, sizeof(ACK), addr); // Mandar ACK para o servidor

                   // O estado muda para Receber
                   estado = Estado::Receber;
                   
                } else if (data->getOpcode() == 5) { // Verifica se recebeu um pacote de erro
                   // Instanciação do pacote ERROR, já atribuindo os bystes recebidos anteriormente
                   error = new ERROR(buffer, bytesAmount);

                   // Salva o código e a mensagem de erro
                   errorCode = error->getErrorCode();
                   errorMessage = error->getErrorMessage();

                   // Dado o erro, o próximo estado é o fim.
                   estado = Estado::Fim;

                   // Como não receberá mais pacotes do servidor,
                   // para que a máquina de estados finalize, é
                   // preciso chamar a mesma mais uma vez
                   start();

                } else {                    // Se acontece qualquer outra coisa
                   estado = Estado::Fim;    // além do esperado, vá para o estado Fim
                   start();  // Chama a máquna de estados uma última vez
                };
                
            }
            break;

        case Estado::Transmitir:
            // Recebimento do possível ACK do servidor após o envio do
            // primeiro pacote DATA
            sock.recv(buffer, 4, addr);
            ack->setBytes(buffer); // Atribui os bytes recebidos no objeto ack

            // Verifica se o pacote recebido realmente é um ACK
            if (ack->getOpcode() == 4) {
                // Se os dados recebidos possuírem 512 bytes, continue a transmissão
                if (data->dataSize() >= 512) {
                    data->increment(); // Incrementar o número de bloco
                    sock.send((char*)data, data->size(), addr); // Enviar o pacote DATA
                } else { // Último pacote
                    sock.send((char*)data, data->size(), addr); // Enviar o último DATA
                    estado = Estado::Fim; // Mudar o estado para o fim
                    transferStatus = true; // Indica que a transferência foi bem sucedida
                    start(); // Chamar a máquina de estados uma última vez
                }

            } else if (data->getOpcode() == 5) { // Verifica se recebeu um pacote de erro
               // Instanciação do pacote ERROR, já atribuindo os bystes recebidos anteriormente
               error = new ERROR(buffer, bytesAmount);

               // Dado o erro, o próximo estado é o fim.
               estado = Estado::Fim;

               // Como não receberá mais pacotes do servidor,
               // para que a máquina de estados finalize, é
               // preciso chamar a mesma mais uma vez
               start();
           
            } else {                  // Se acontece qualquer outra coisa
               estado = Estado::Fim;  // além do esperado, vá para o estado Fim
               start();  // Chama a máquna de estados uma última vez
            }

            break;
        
        case Estado::Receber:
            // Recebimento do possível DATA do servidor
            // // bytesAmount armazena a quantidade de bytes recebidos
            bytesAmount = sock.recv(buffer, 516, addr);
            data = new DATA(buffer, bytesAmount); // Salvar bytes do DATA no objeto data
 
            // Verifica se o pacote recebido realmente é um DATA
            if (data->getOpcode() == 3){ 
               // Escrever os dados recebidos no arquivo
               outputFile->write(data->getData(), data->dataSize());

               // Mandar um ACK para o servidor
               ack->increment(); // Incrementar o número de bloco do ack
               sock.send((char*)ack, sizeof(ACK), addr); // Enviar o ACK

               // Se o trecho do arquivo recebido for menor do que 512 bytes
               // o estado é definido para o Fim
               if (data->dataSize() < 512) {
                   outputFile->close(); // O arquivo é sincronizado no armazenamento
                   estado = Estado::Fim;  // o estado é definido para o Fim
                   transferStatus = true;
                   start(); // A máquina de estados é chamada uma última vez
               }
  
            } else if (data->getOpcode() == 5) { // Verifica se recebeu um pacote de erro
               // Instanciação do pacote ERROR, já atribuindo os bystes recebidos anteriormente
               error = new ERROR(buffer, bytesAmount);

               // Dado o erro, o próximo estado é o fim.
               estado = Estado::Fim;
               start(); // A máquina de estados é chamada uma última vez

            } else { // Para qualquer outro evento o estado é o fim
               estado = Estado::Fim;
               start();
            }
            break;
        
        case Estado::Fim:
            // Finaliza a rotina do poller
            finish();
            break;
    }
}

void TFTP::handle_timeout() {
    start();
}

void TFTP::start() {
    handle();
}

bool TFTP::status(){
    return transferStatus;
}

uint16_t TFTP::getErrorCode(){
    return errorCode;
}

string TFTP::getErrorMessage(){
    return errorMessage;
}
