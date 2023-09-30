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
    TFTP::Operation operation;

    // Seleciona a operação com base na entrada do usuário
    if (operacao == "enviar"){
        cout << "Enviando o arquivo \"" << arq_origem << "\" para o servidor como \"" << arq_destino << "\"..." << endl;
        operation = TFTP::SEND;
    } else if (operacao == "receber"){
        cout << "Recebendo o arquivo \"" << arq_origem << "\" do servidor e salvando como \"" << arq_destino << "\"..." << endl;
        operation = TFTP::RECEIVE;
    } else {
        cout << "Operação inválida!!!" << endl;
        exit(1);
    }

    Poller sched;

    sockpp::UDPSocket sock;
    sockpp::AddrInfo addr(end_servidor, porta);

    // Cria um objeto TFTP com timeout de 3 segundos
    TFTP cb_tftp(sock, addr, 3, operation, arq_origem, arq_destino);

    // Adiciona o objeto TFTP ao poller
    sched.adiciona(&cb_tftp);
    sched.despache();
    
    // Verifica se o processo de transferência foi bem sucedido
    if (cb_tftp.status()) cout << "Tudo certo" << endl;
    else {
        cout << "Erro " << cb_tftp.getErrorCode() << ": " << cb_tftp.getErrorMessage() << endl;
        exit(1);
    }

    return 0;
}
