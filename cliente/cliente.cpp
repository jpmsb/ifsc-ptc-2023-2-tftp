#include "poller.h"
#include "UDPSocket.h"
#include <iostream>
#include <string>
#include "TFTP.h"

using namespace std;

int main(int argc, char * argv[]) {
    string end_servidor = argv[1];
    int porta = stoi(argv[2]);
    string operacao = argv[3];
    string arq_origem = argv[4];
    string arq_destino = argv[5];

    Poller sched;

    sockpp::UDPSocket sock;
    sockpp::AddrInfo addr(end_servidor, porta);

    TFTP cb_tftp(sock, addr, 0, operacao, arq_origem, arq_destino);

    sched.adiciona(&cb_tftp);

    sched.despache();


}
