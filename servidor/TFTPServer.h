#include "poller.h"
#include "UDPSocket.h"
#include <string>
#include "Request.h"
#include "DATA.h"
#include "ACK.h"
#include "ERROR.h"
#include <unistd.h>
#include "tftp2.pb.h"
#include <sys/stat.h>
#include <dirent.h>
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
  int moveElement(string oldName, string newName);
  int listDirectoryContents(string path, tftp2::ListResponse * listResponse);
  ERROR* createErrorFromSysCallError(int errorNumber);
  void resetAll();
  void clearAll();
  void handle();
  void handle_timeout();
  void start();
  enum Estado {
      Espera,
      Transmitir,
      Receber,
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
