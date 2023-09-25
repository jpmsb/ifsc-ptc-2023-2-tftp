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
class ComunicaComServidor: public Callback {

 public:

 ComunicaComServidor(sockpp::UDPSocket & sock, sockpp::AddrInfo & addr, string & operacao, string & arq_remoto, string & arq_local)
    : Callback(sock.get_descriptor(), 0), sock(sock), addr(addr), operation(operacao), remoteFile(arq_remoto), localFile(arq_local) {
            disable_timeout();
            estado = Estado::Inicio;
	    start(); // inicializa primeiro pacote
  }

  void handle() {
      switch (estado) {
          case Estado::Inicio:
              cout << "Início!!" << endl;
              if (operation == "enviar"){
                  wrq = new WRQ(remoteFile);
              } else if (operation == "receber"){
                  rrq = new RRQ(remoteFile);
              }
              estado = Estado::Conexao;

              break;

          case Estado::Conexao:
              cout << "Conexão!!" << endl;
              if (operation == "enviar"){
                  sock.send(wrq->data(), wrq->size(), addr);
                  estado = Estado::Transmitir;
                  data = new DATA(remoteFile);
              } else if (operation == "receber"){
                  sock.send(rrq->data(), rrq->size(), addr);
                  estado = Estado::Receber;
              }
              break;

          case Estado::Transmitir:
              cout << "Transmitindo \"" << data->size() << "\" bytes..." << endl;

              if (data->size() >= 512) {
                  sock.send((char*)&data, data->size(), addr);
                  data->increment();
              } else { // Último pacote
                  sock.send((char*)&data, data->size(), addr);
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
      Inicio,
      Conexao,
      Transmitir,
      Receber,
      Fim
  };
  Estado estado;
  sockpp::UDPSocket & sock;
  sockpp::AddrInfo addr;
  string operation;
  string remoteFile;
  string localFile;
  RRQ * rrq;
  WRQ * wrq;
  DATA * data;
  ACK * ack;

};

int main(int argc, char * argv[]) {
    string end_servidor = argv[1];
    int porta = stoi(argv[2]);
    string operacao = argv[3];
    string arquivo_remoto = argv[4];
    string arquivo_local = argv[5];

    Poller sched;

    sockpp::UDPSocket sock;
    sockpp::AddrInfo addr(end_servidor, porta);

    ComunicaComServidor cb_servidor(sock, addr, operacao, arquivo_remoto, arquivo_local);

    sched.adiciona(&cb_servidor);

    cb_servidor.handle();

    sched.despache();

    cout << "aaaaa" << endl;

}
