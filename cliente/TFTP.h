#include "poller.h"
#include "UDPSocket.h"
#include <string>
#include "RRQ.h"
#include "WRQ.h"
#include "DATA.h"
#include "ACK.h"
#include "ERROR.h"
#include "tftp2.pb.h"

using namespace std;

class TFTP: public Callback {
 public:
 // Enumeração que organiza as operações
 enum Operation {
     SEND,
     RECEIVE,
     LIST,
     MKDIR,
     MOVE
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
      Resultado,
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
  uint8_t timeoutCounter;
  bool timeoutState;
  tftp2::Mensagem * pbMessage;
  string serializedMessage;
  string listingOutput;
};
