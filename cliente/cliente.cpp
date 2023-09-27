#include "poller.h"
#include "UDPSocket.h"
#include <iostream>
#include <string>
#include "RRQ.h"
#include "WRQ.h"
#include "ACK.h"
#include "DATA.h"

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
                  char buffer[4];
                  sock.recv(buffer, 4, addr);
                  
                  ack = new ACK();
                  ack->setBytes(buffer); // Salvar bytes do ACK no objeto ack

                  if (ack->getOpcode() == 4) {
                      ack->setBytes(buffer);
                      cout << "(Conexão) Recebeu ACK " << ack->getBlock() << "!" << endl;
                      data->increment();
                      sock.send((char*)data, data->size(), addr);
                      estado = Estado::Transmitir;

                  } else cout << "O pacote recebido não é um ACK!" << endl;


              } else if (operation == "receber"){
                  cout << "Recebendo o arquivo \"" << srcFile << "\"!!" << endl;
                  sock.send(rrq->data(), rrq->size(), addr);
                  estado = Estado::Receber;

              }
              break;

          case Estado::Transmitir:
              char buffer[4];
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
