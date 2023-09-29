#include "poller.h"
#include "UDPSocket.h"
#include <iostream>
#include <string>
#include "RRQ.h"
#include "WRQ.h"
#include "DATA.h"
#include "ACK.h"
#include "ERROR.h"

using namespace std;

// A especialização de Callback para se comunicar com o servidor
class TFTP: public Callback {

 public:

 TFTP(sockpp::UDPSocket & sock, sockpp::AddrInfo & addr, string & operacao, string & sourceFile, string & destinationFile)
    : Callback(sock.get_descriptor(), 0), sock(sock), addr(addr), operation(operacao), srcFile(sourceFile), destFile(destinationFile) {
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

  void handle() {
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

                  } else cout << "O pacote recebido não é um ACK!" << endl;


              } else if (operation == "receber"){
                  cout << "Recebendo o arquivo \"" << srcFile << "\"!!" << endl;
                  sock.send(rrq->data(), rrq->size(), addr);

                  // Preparando para receber DATA
                  bytesAmount = sock.recv(buffer, 516, addr);
                  data = new DATA(buffer, bytesAmount); // Salvar bytes do DATA no objeto data

                  if (data->getOpcode() == 3){
                     cout << "(Conexão) Recebeu DATA " << data->getBlock() << "!" << endl;
                     outputFile = new ofstream(srcFile);

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

                  } else cout << "O pacote recebido não é um DATA!" << endl;
                  
              }
              break;

          case Estado::Transmitir:
              sock.recv(buffer, 4, addr);

              if (ack->getOpcode() == 4) {
                  ack->setBytes(buffer);
                  cout << "Recebeu ACK " << ack->getBlock() << "!" << endl;
                  data->increment();
              }
              else cout << "O pacote recebido não é um ACK!" << endl;

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
   
              cout << "Recebeu " << endl;
 
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

              } else cout << "O pacote recebido não é um DATA!" << endl;
              break;
          
          case Estado::Fim:
              cout << "Fim!" << endl;
              exit(0);
              break;
      }
  }

  void handle_timeout() {
  }

  void start() {
      handle();
  };

 private:
  enum Estado {
      Conexao,
      Transmitir,
      Receber,
      Fim
  };
  Estado estado;
  sockpp::UDPSocket & sock;
  sockpp::AddrInfo addr;
  string operation;
  string srcFile;
  string destFile;
  RRQ * rrq;
  WRQ * wrq;
  DATA * data;
  ACK * ack;
  ERROR * error;
  ofstream * outputFile;
  char buffer[516];
  int bytesAmount;
};

int main(int argc, char * argv[]) {
    string end_servidor = argv[1];
    int porta = stoi(argv[2]);
    string operacao = argv[3];
    string arq_origem = argv[4];
    string arq_destino = argv[5];

    Poller sched;

    sockpp::UDPSocket sock;
    sockpp::AddrInfo addr(end_servidor, porta);

    TFTP cb_tftp(sock, addr, operacao, arq_origem, arq_destino);

    sched.adiciona(&cb_tftp);

    sched.despache();


}
