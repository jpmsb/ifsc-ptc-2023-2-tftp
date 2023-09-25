#include <iostream>
#include <istream>
#include <string.h>
#include <arpa/inet.h>
#include "ACK.h"
#include "DATA.h"
#include "RRQ.h"
#include "WRQ.h"
#include "UDPSocket.h"

using namespace std;

int main(int argc, char * argv[]){
    sockpp::UDPSocket sock;
    sockpp::AddrInfo addr("127.0.0.1", 6969);
    
    char buffer[512];

    ifstream arquivo(argv[1]);

    string arq_origem = argv[1];
    ACK ack;
    ack.increment();

    WRQ rrq(arq_origem);

    // ifstream arquivo(arq_origem, std::ios::binary);
    // arquivo.read(buffer, 512);
    // streamsize tamanho = arquivo.gcount();

    // DATA data(*buffer);
    DATA data(arq_origem);
    // data.increment();
    // data.increment();

    cout << "data block " << data.getBlock() << endl;
    cout << "data opcode " << data.getOpcode() << endl;
    cout << "data dataSize " << data.dataSize() << endl;

    sock.send(rrq.data(), rrq.size(), addr);
    // sock.send((char*)&ack, sizeof(ACK), addr);
    sock.send((char*)&data, data.size(), addr);

    union {
      unsigned char uc4[4];
      unsigned long ul;
    } bytes;
    memcpy(&(bytes.uc4), &ack, 4);

    cout << hex << (int)bytes.uc4[0];
    cout << hex << (int)bytes.uc4[1];
    cout << hex << (int)bytes.uc4[2];
    cout << hex << (int)bytes.uc4[3] << endl;
}
