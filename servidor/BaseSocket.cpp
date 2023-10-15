/* 
 * File:   BaseSocket.cpp
 * Author: msobral
 * 
 * Created on 6 de Julho de 2016, 16:14
 */

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include "BaseSocket.h"

namespace sockpp {

    sockaddr_in BaseSocket::make_addr(const AddrInfo &addr) const {
        sockaddr_in myaddr;
        hostent *haddr = gethostbyname(addr.addr.c_str());

        if (not haddr) throw BaseSocket::SocketException(errno);

        myaddr.sin_family = AF_INET;
        memcpy(&myaddr.sin_addr, haddr->h_addr_list[0], haddr->h_length);
        myaddr.sin_port = htons(addr.port);

        return myaddr;
    }

    void BaseSocket::init(int type, int proto, const AddrInfo &addr) {
        sockaddr_in myaddr = make_addr(addr);

        if ((sd = socket(AF_INET, type, proto)) < 0) {
            throw BaseSocket::SocketException(errno);
        }

        int opt = 1;
        if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (void *) &opt, sizeof(opt)) < 0) {
            throw BaseSocket::SocketException(errno);
        }
/*    if (fcntl(sd, F_SETFL, O_NONBLOCK) < 0) {
        throw BaseSocket::SocketException(errno);        
    }
*/
        if (bind(sd, (sockaddr *) &myaddr, sizeof(myaddr)) < 0) {
            throw BaseSocket::SocketException(errno);
        }
    }

    BaseSocket::BaseSocket() : sd(0) {
    }

    BaseSocket::BaseSocket(const BaseSocket &orig) {
        sd = orig.sd;
    }

    BaseSocket::BaseSocket(const AddrInfo &addr) {
    }

    BaseSocket::BaseSocket(int socket_descriptor) : sd(socket_descriptor) {
        if (fcntl(sd, F_SETFL, O_NONBLOCK) < 0) {
            throw BaseSocket::SocketException(errno);
        }
    }

    BaseSocket::~BaseSocket() {
        if (sd) ::close(sd);
    }

    AddrInfo BaseSocket::get_peer() const {
        sockaddr_in paddr;
        socklen_t len = sizeof(paddr);

        if (getpeername(sd, (sockaddr *) &paddr, &len) < 0) {
            throw BaseSocket::SocketException(errno);
        }

        return AddrInfo{inet_ntoa(paddr.sin_addr), ntohs(paddr.sin_port)};
    }

    AddrInfo BaseSocket::get_self() const {
        sockaddr_in paddr;
        socklen_t len = sizeof(paddr);

        if (getsockname(sd, (sockaddr *) &paddr, &len) < 0) {
            throw BaseSocket::SocketException(errno);
        }

        return AddrInfo{inet_ntoa(paddr.sin_addr), ntohs(paddr.sin_port)};
    }

    void BaseSocket::connect(const AddrInfo &addr) {
        sockaddr_in saddr = make_addr(addr);

        if (::connect(sd, (sockaddr *) &saddr, sizeof(saddr)) < 0) {
            throw BaseSocket::SocketException(errno);
        }
    }

    int BaseSocket::send(std::string_view buffer) {
        return send(buffer.data(), buffer.size());
    }

    int BaseSocket::send(const std::vector<char> &buffer) {
        return send(buffer.data(), buffer.size());
    }

    int BaseSocket::send(const char *buffer, int num_bytes) {
        if (not sd) throw BaseSocket::SocketException(EINVAL);

        if (not num_bytes) return 0;

        int n = ::send(sd, buffer, num_bytes, 0);

        if (n < 0) throw BaseSocket::SocketException(errno);

        return n;
    }

    std::vector<char> BaseSocket::recv(int max_bytes) {
        std::vector<char> buffer(max_bytes, 0);

        int n = recv(buffer.data(), max_bytes);
        buffer.resize(n);
        return buffer;
    }

    int BaseSocket::recv(char *buffer, int max_bytes) {
        if (not sd) throw BaseSocket::SocketException(EINVAL);

        int n = ::recv(sd, buffer, max_bytes, 0);
        if (n < 0) {
            if (errno != EAGAIN) throw BaseSocket::SocketException(errno);
            n = 0;
        }
        return n;
    }

    bool BaseSocket::hasData() const {
        char x;
        struct timeval tv = {0, 0};
        fd_set socks;
        FD_ZERO(&socks);
        FD_SET(sd, &socks);

        int n = select(sd + 1, &socks, NULL, NULL, &tv);
        if (n) {
            ssize_t r = ::recv(sd, &x, 1, MSG_DONTWAIT | MSG_PEEK);
            return (r > 0);
        }
        return false;
    }

}
