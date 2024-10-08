#include "poller.h"
#include "UDPSocket.h"
#include <iostream>
#include <string>
#include "TFTPServer.h"

using namespace std;

int main(int argc, char * argv[]) {
    if (argc != 3) {
        cerr << "Uso: " << argv[0] << " <porta> <diretorio_raiz>" << endl;
        return 1;
    }

    int porta = stoi(argv[1]);
    string diretorio_raiz = argv[2];

    Poller sched;

    sockpp::AddrInfo addr(porta);
    sockpp::UDPSocket sock(addr);

    // Cria um objeto TFTP com timeout de 3 segundos
    cout << "Servidor iniciado: " << endl;
    cout << "Porta: " << porta << endl;
    cout << "Diretorio raiz: " << diretorio_raiz << endl;
    
    TFTPServer * cb_tftp = new TFTPServer(sock, addr, 3000, diretorio_raiz);

    // Adiciona o objeto TFTP ao poller
    sched.adiciona(cb_tftp);
    sched.despache();

    return 0;
}
