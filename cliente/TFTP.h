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

class TFTP: public Callback {
 public:
  TFTP(sockpp::UDPSocket & sock, sockpp::AddrInfo & addr, int timeout, string & operacao, string & sourceFile, string & destinationFile);

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
