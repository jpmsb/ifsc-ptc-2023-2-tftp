#include "poller.h"
#include "UDPSocket.h"
#include <string>
#include "RRQ.h"
#include "WRQ.h"
#include "DATA.h"
#include "ACK.h"
#include "ERROR.h"

using namespace std;

class TFTP: public Callback {
 public:
 // Enumeração que organiza as operações de envio e recebimento
 enum Operation {
     SEND,
     RECEIVE,
 };

  // Construtor da classe que recebe todos os parâmetros
  // para o funcionamento do cliente TFTP
  TFTP(sockpp::UDPSocket & sock, sockpp::AddrInfo & addr, int timeout, Operation operation, string & sourceFile, string & destinationFile);
  ~TFTP();

 private:
  void handle();
  void handle_timeout();
  void start();
  enum Estado {
      Conexao,
      Transmitir,
      Receber,
      Fim
  };
  Estado estado;
  sockpp::UDPSocket & sock;
  sockpp::AddrInfo addr;
  Operation operation;
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
