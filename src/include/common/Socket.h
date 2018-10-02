#pragma once

#include "SocketException.h"
#include <netinet/in.h>      // For sockaddr_in
#include <sys/socket.h>      // For socket(), connect(), send(), and recv()

namespace wavefront {
    /**
    *   Base class representing basic communication endpoint
    *   @author Mengran Wang (mengranw@vmware.com)
    */
    class Socket {
    public:
        Socket(int type, int protocol) throw(SocketException);

        /**
         *   Close and deallocate this socket
         */
        ~Socket();

        /**
         *   @return host name of socket
         *   @exception SocketException thrown if fetch fails
         */
        std::string getLocalAddress() throw(SocketException);

        /**
         *   Get the local port
         *   @return local port of socket
         *   @exception SocketException thrown if fetch fails
         */
        unsigned short getLocalPort() throw(SocketException);

        /**
         *   Set the local port to the specified port and the local address
         *   to any interface
         *   @param localPort local port
         *   @exception SocketException thrown if setting local port fails
         */
        void setLocalPort(unsigned short localPort) throw(SocketException);

        /**
         *   Set the local port to the specified port and the local address
         *   to the specified address.  If you omit the port, a random port
         *   will be selected.
         *   @param localAddress local address
         *   @param localPort local port
         *   @exception SocketException thrown if setting local port or address fails
         */
        void setLocalAddressAndPort(const std::string &localAddress,
                                    unsigned short localPort = 0) throw(SocketException);

        /**
         *   Resolve the specified service for the specified protocol to the
         *   corresponding port number in host byte order
         *   @param service service to resolve (e.g., "http")
         *   @param protocol protocol of service to resolve.  Default is "tcp".
         */
        static unsigned short resolveService(const std::string &service,
                                             const std::string &protocol = "tcp");

        /**
        *   close underlying socket
        */
        void close() throw(SocketException);

    private:
        // Prevent the user from trying to use value semantics on this object
        Socket(const Socket &sock);

        void operator=(const Socket &sock);

    protected:
        int sockDesc = -1;              // Socket descriptor

        unsigned short port = 0;                  // host name and port
        std::string host;

        Socket(int sockDesc);
    };

/**
 *   Socket which is able to connect, send, and receive
 */
    class CommunicatingSocket : public Socket {
    public:
        CommunicatingSocket(int type = SOCK_STREAM, int protocol = IPPROTO_TCP) throw(SocketException);

        /**
         *   Establish a socket connection with the given foreign
         *   address and port
         *   @param foreignAddress foreign address (IP address or name)
         *   @param foreignPort foreign port
         *   @exception SocketException thrown if unable to establish connection
         */
        void connect(const std::string &foreignAddress, unsigned short foreignPort) throw(SocketException);

        /**
         *   Write the given buffer to this socket.  Call connect() before
         *   calling send()
         *   @param buffer buffer to be written
         *   @param bufferLen number of bytes from buffer to be written
         *   @exception SocketException thrown if unable to send data
         */
        void send(const char *buffer, int bufferLen) throw(SocketException);


    protected:
        CommunicatingSocket(int newConnSD);
    };
}

