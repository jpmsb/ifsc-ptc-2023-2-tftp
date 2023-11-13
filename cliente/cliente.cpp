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
    string arq_origem = "";
    string arq_destino = "";

    if (argc > 4){
	arq_origem = argv[4];
    }

    if (argc > 5){
        arq_destino = argv[5];
    }

    TFTP::Operation operation;

    // Seleciona a operação com base na entrada do usuário
    if (operacao == "enviar"){
        cout << "Enviando o arquivo \"" << arq_origem << "\" para o servidor como \"" << arq_destino << "\"..." << endl;
        operation = TFTP::SEND;
    } else if (operacao == "receber"){
        cout << "Recebendo o arquivo \"" << arq_origem << "\" do servidor e salvando como \"" << arq_destino << "\"..." << endl;
        operation = TFTP::RECEIVE;
    } else if (operacao == "listar"){
	cout << "Listando os arquivos do servidor..." << endl;
	operation = TFTP::LIST;
    } else if (operacao == "mover"){
        if (arq_destino.size() > 0) {
            cout << "Renomeando o arquivo \"" << arq_origem << "\" para \"" << arq_destino << "\"..." << endl;
        } else {
            cout << "Removendo o arquivo \"" << arq_origem << "\"..." << endl;
        }
        operation = TFTP::MOVE;
    } else if (operacao == "criardir"){
        cout << "Criando o diretório \"" << arq_origem << "\"..." << endl;
	operation = TFTP::MKDIR;
    } else {
        cout << "Operação inválida!!!" << endl;
        return 1;
    }

    Poller sched;

    sockpp::UDPSocket sock;
    sockpp::AddrInfo addr(end_servidor, porta);

    try {
        // Cria um objeto TFTP com timeout de 3 segundos
        TFTP * cb_tftp = new TFTP(sock, addr, 3000, operation, arq_origem, arq_destino);

        // Adiciona o objeto TFTP ao poller
        sched.adiciona(cb_tftp);

        sched.despache();
        cout << "Transação realizada com sucesso!!" << endl;
    } catch(ERROR * e){
        cerr << "Erro " << e->getErrorCode() << ": " << e->getErrorMessage() << endl;
    } catch (string & e) {
	cerr << "Erro: " << e << endl;
    }
   
    return 0;
}
