#include "poller.h"
#include "UDPSocket.h"
#include <string>
#include "Request.h"
#include "DATA.h"
#include "ACK.h"
#include "ERROR.h"
#include <unistd.h>
#include "tftp2.pb.h"
#include <filesystem>
#include <sys/stat.h>
#include <iostream>

using namespace std;

class TFTPServer: public Callback {
 public:
  // Construtor da classe que recebe todos os parâmetros
  // para o funcionamento do servidor TFTPServer
  TFTPServer(sockpp::UDPSocket & sock, sockpp::AddrInfo & addr, int timeout, string rootDirectory);
  ~TFTPServer();

 private:
  int createDirectory(string path);
  void resetAll();
  void clearAll();
  void handle();
  void handle_timeout();
  void start();
  enum Estado {
      Espera,
      Transmitir,
      Receber,
      Listar,
      CriarDiretorio,
      Mover,
      Fim
  };
  Estado estado;
  sockpp::UDPSocket & sock;
  sockpp::AddrInfo & addr;
  string rootDir;
  Request * request;
  DATA * data;
  ACK * ack;
  ERROR * error;
  ofstream * outputFile;
  char buffer[516];
  int bytesAmount;
  uint8_t timeoutCounter;
  bool timeoutState;
  tftp2::Mensagem * desserializedMessage;
};
