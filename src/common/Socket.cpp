#include "common/Socket.h"
#include <sys/types.h>       // For data types
#include <netdb.h>           // For gethostbyname()
#include <arpa/inet.h>       // For inet_addr()
#include <unistd.h>          // For close()
#include <netinet/in.h>      // For sockaddr_in
#include <arpa/inet.h>       // For inet_addr()
#include <iostream>

namespace wavefront {
    // Function to fill in address structure given an address and port
    static void fillAddr(const std::string &address, unsigned short port,
                         sockaddr_in &addr) {
        memset(&addr, 0, sizeof(addr));  // Zero out address structure
        addr.sin_family = AF_INET;       // Internet address

        hostent *host;  // Resolve name
        if ((host = gethostbyname(address.c_str())) == NULL) {
            // strerror() will not work for gethostbyname() and hstrerror()
            // is supposedly obsolete
            throw SocketException("Failed to resolve name (gethostbyname())");
        }
        addr.sin_addr.s_addr = *((unsigned long *) host->h_addr_list[0]);

        addr.sin_port = htons(port);     // Assign port in network byte order
    }

    Socket::Socket(int type, int protocol) throw(SocketException) {
        // Make a new socket
        if ((sockDesc = socket(PF_INET, type, protocol)) < 0) {
            throw SocketException("Socket creation failed (socket())", true);
        }
    }

    Socket::Socket(int sockDesc) {
        this->sockDesc = sockDesc;
    }

    Socket::~Socket() {
        if (sockDesc != -1) {
            close();
        }
    }

    std::string Socket::getLocalAddress() throw(SocketException) {
        sockaddr_in addr;
        unsigned int addr_len = sizeof(addr);

        if (getsockname(sockDesc, (sockaddr *) &addr, (socklen_t *) &addr_len) < 0) {
            throw SocketException("Fetch of local address failed (getsockname())", true);
        }
        return inet_ntoa(addr.sin_addr);
    }

    unsigned short Socket::getLocalPort() throw(SocketException) {
        sockaddr_in addr;
        unsigned int addr_len = sizeof(addr);

        if (getsockname(sockDesc, (sockaddr *) &addr, (socklen_t *) &addr_len) < 0) {
            throw SocketException("Fetch of local port failed (getsockname())", true);
        }
        return ntohs(addr.sin_port);
    }

    void Socket::setLocalPort(unsigned short localPort) throw(SocketException) {
        // Bind the socket to its port
        sockaddr_in localAddr;
        memset(&localAddr, 0, sizeof(localAddr));
        localAddr.sin_family = AF_INET;
        localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        localAddr.sin_port = htons(localPort);

        if (bind(sockDesc, (sockaddr *) &localAddr, sizeof(sockaddr_in)) < 0) {
            throw SocketException("Set of local port failed (bind())", true);
        }
    }

    void Socket::setLocalAddressAndPort(const std::string &localAddress,
                                        unsigned short localPort) throw(SocketException) {
        // Get the address of the requested host
        sockaddr_in localAddr;
        fillAddr(localAddress, localPort, localAddr);

        if (bind(sockDesc, (sockaddr *) &localAddr, sizeof(sockaddr_in)) < 0) {
            throw SocketException("Set of local address and port failed (bind())", true);
        }
    }

    unsigned short Socket::resolveService(const std::string &service,
                                          const std::string &protocol) {
        struct servent *serv;        /* Structure containing service information */

        if ((serv = getservbyname(service.c_str(), protocol.c_str())) == NULL)
            return atoi(service.c_str());  /* Service is port number */
        else
            return ntohs(serv->s_port);    /* Found port (network byte order) by name */
    }

    void Socket::close() throw(SocketException) {
        if (sockDesc != -1) {
            if (::close(sockDesc) < 0) {
                throw SocketException("Close failed (close())", true);
            }
            sockDesc = -1;
        }
    }

    CommunicatingSocket::CommunicatingSocket(int type, int protocol) throw(SocketException) : Socket(type, protocol) {
    }

    CommunicatingSocket::CommunicatingSocket(int newConnSD) : Socket(newConnSD) {
    }

    void CommunicatingSocket::connect(const std::string &foreignAddress,
                                      unsigned short foreignPort) throw(SocketException) {
        // Get the address of the requested host
        sockaddr_in destAddr;
        fillAddr(foreignAddress, foreignPort, destAddr);

        // Try to connect to the given port
        if (::connect(sockDesc, (sockaddr *) &destAddr, sizeof(destAddr)) < 0) {
            throw SocketException("Connect failed (connect())", true);
        }
    }

    void CommunicatingSocket::send(const char *buffer, int bufferLen)
    throw(SocketException) {
        if (::send(sockDesc, (void *) buffer, bufferLen, 0) < 0) {
            throw SocketException("Send failed (send())", true);
        }
    }
}

