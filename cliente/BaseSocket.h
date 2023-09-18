/* 
 * File:   BaseSocket.h
 * Author: msobral
 *
 * Created on 6 de Julho de 2016, 16:14
 */

#ifndef BaseSOCKET_H
#define	BaseSOCKET_H

#include <string>
#include <list>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <string_view>

namespace sockpp {
    struct AddrInfo {
        std::string addr;
        uint16_t port;

        AddrInfo() : addr("0.0.0.0"), port(0) {}

        AddrInfo(uint16_t port) : addr("0.0.0.0"), port(port) {}

        AddrInfo(const std::string &addr, uint16_t port = 0) : addr(addr), port(port) {}

        std::string str() const {
            return addr + ':' + std::to_string(port);
        }

        bool is_any() const {
            return port == 0 && addr == "0.0.0.0";
        }

        bool operator==(const AddrInfo & outro) const {
            return addr == outro.addr && port == outro.port;
        }
        bool operator!=(const AddrInfo & outro) const {
            return addr != outro.addr || port != outro.port;
        }
    };

// classe BaseSocket: não deve ser instanciada diretamente ..
// pois é a classe base para BaseClientSocket e BaseServerSocket
    class BaseSocket {
    public:
        BaseSocket(const AddrInfo &addr);

        BaseSocket(int socket_descriptor);

        BaseSocket();

        BaseSocket(const BaseSocket &orig);

        virtual ~BaseSocket();

        // obtém endereço IP e port do socket da outra ponta
        // da conexão
        AddrInfo get_peer() const;

        // obtém endereço IP e port deste socket
        AddrInfo get_self() const;

        // conecta a um peer
        virtual void connect(const AddrInfo &addr);

        // envia os "numbytes" bytes contidos em "buffer"
        int send(const char *buffer, int num_bytes);

        int send(std::string_view buffer);

        int send(const std::vector<char> &buffer);

        // recebe até "max_bytes", e retorna-os como um vector
        std::vector<char> recv(int max_bytes);

        // recebe até "max_bytes", e grava-os em "buffer"
        int recv(char *buffer, int max_bytes);

        // retorna o descritor deste socket
        int get_descriptor() const { return sd; }

        // testa se o há dados para ler no socket
        bool hasData() const;

        // classe SocketException: usada para informar erros
        // de socket em geral. O código de erro
        // segue os erros padrões da variável errno
        class SocketException : public std::exception {
        public:
            SocketException(int error) : erro(error) {}

            ~SocketException() throw() {}

            const char *what() { return strerror(erro); }

            int get_errno() { return erro; }

        private:
            int erro;
        };
        // classe AddrException: usada para informar erros
        // de endereço de socketg (addrInfo).
        class AddrException : public std::exception {
        public:
            AddrException(const std::string & msg) : msg(msg) {}

            ~AddrException() throw() {}

            const char *what() { return msg.c_str(); }

        private:
            const std::string msg;
        };

    protected:
        int sd;

        void init(int type, int proto, const AddrInfo &addr);

        sockaddr_in make_addr(const AddrInfo &addr) const;
    };
}
#endif	/* BaseSOCKET_H */

