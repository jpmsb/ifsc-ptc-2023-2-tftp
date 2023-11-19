#include "poller.h"
#include "UDPSocket.h"
#include <iostream>
#include <string>
#include "TFTP.h"
#include "tftp2.pb.h"
#include <list>
#include <unordered_map>

using namespace std;

int main(int argc, char * argv[]) {
    string end_servidor = argv[1];
    int porta = stoi(argv[2]);
    string operacao = argv[3];
    string arq_origem = ".";
    string arq_destino = "";
    unordered_map<string, string> * arquivos;
    list<string> * diretorios;
    int fileCharacterAmount = 0;
    int sizeCharacterAmount = 0;
    string verde = "\033[1;32m";
    string magenta = "\033[1;35m";
    string ciano = "\033[1;36m";
    string vermelho = "\033[1;31m";
    string amarelo = "\033[1;33m";
    string normal = "\033[0m";
    
    if (argc > 4){
	arq_origem = argv[4];
    }

    if (argc > 5){
        arq_destino = argv[5];
    }

    TFTP::Operation operation;

    // Seleciona a operação com base na entrada do usuário
    if (operacao == "listar" || operacao == "ls"){
        cout << "Listando o conteúdo do diretório \"" << arq_origem << "\"\n";
        operation = TFTP::LIST;
    } else if (arq_origem != "." && arq_destino != "." && arq_origem != ".." && arq_destino != ".." && arq_origem != ""){
        if (operacao == "enviar"){
            cout << "Enviando o arquivo \"" << arq_origem << "\" para o servidor como \"" << arq_destino << "\"..." << endl;
            operation = TFTP::SEND;
        } else if (operacao == "receber"){
            cout << "Recebendo o arquivo \"" << arq_origem << "\" do servidor e salvando como \"" << arq_destino << "\"..." << endl;
            operation = TFTP::RECEIVE;
        } else if (operacao == "mover" || operacao == "mv"){
            if (arq_destino.size() > 0) {
                cout << "Renomeando o arquivo \"" << arq_origem << "\" para \"" << arq_destino << "\"..." << endl;
            } else {
                cout << "Removendo o arquivo \"" << arq_origem << "\"..." << endl;
            }
            operation = TFTP::MOVE;
        } else if (operacao == "criardir" || operacao == "mkdir"){
            cout << "Criando o diretório \"" << arq_origem << "\"..." << endl;
            operation = TFTP::MKDIR;
        } else {
            cout << "Operação inválida!!!" << endl;
	    return 1;
        }
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
        cout << verde + "Transação realizada com sucesso!!" + normal << endl;
    } catch(ERROR * e){
        cerr << vermelho << "Erro " << e->getErrorCode() << ": " << e->getErrorMessage() << normal << endl;
    } catch (string & e) {
        if (operation == TFTP::LIST){
            tftp2::Mensagem pbMessage;
            pbMessage.ParseFromString(e);

            if (pbMessage.has_list_response()){
                for (const auto & item : pbMessage.list_response().items()){
                    if (item.has_directory()){
                        if (diretorios == nullptr) diretorios = new list<string>();

                        diretorios->push_back(ciano + item.directory().path() + normal);
                    } else if (item.has_file()){
                        if (arquivos == nullptr) arquivos = new unordered_map<string, string>();
                        string nomeArq = magenta + item.file().name() + normal + ",";
                        string tamanhoArq = amarelo + "tamanho: " + to_string(item.file().size()) + normal + " bytes";

                        arquivos->insert({nomeArq, tamanhoArq});

                        if (nomeArq.size() > fileCharacterAmount) fileCharacterAmount = nomeArq.size();
                        if (tamanhoArq.size() > sizeCharacterAmount) sizeCharacterAmount = tamanhoArq.size();
                    }
                }

                if (diretorios != nullptr){
                    cout << "\nDiretórios:\n";
                    for (const auto & dir : *diretorios){
                        cout << dir << "/" << endl;
                    }
                }

                if (arquivos != nullptr){
                    cout << "\nArquivos:\n";
                    for (const auto & arq : *arquivos){
                        cout << setw(fileCharacterAmount) << left << arq.first << setw(sizeCharacterAmount) << left << arq.second << endl;
                    }
                }

                cout << verde + "\nListagem realizada com sucesso!!" + normal << endl;
            } else {
                cerr << vermelho + "Mensagem não identificada!" + normal << endl;
            } 
        } else {
            cerr << vermelho << "Erro: " << e << normal << endl;
        }
    }
    return 0;
}
