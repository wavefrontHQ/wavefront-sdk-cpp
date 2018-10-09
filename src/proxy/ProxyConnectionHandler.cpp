#include "proxy/ProxyConnectionHandler.h"

#include <memory>

namespace wavefront {
    ProxyConnectionHandler::ProxyConnectionHandler(std::string &hostName, unsigned short port)
            : hostName(hostName),
              port(port),
              socket(new CommunicatingSocket()) {
    }

    ProxyConnectionHandler::~ProxyConnectionHandler() {
        if (socket != nullptr) {
            close();
        }
    }

    void ProxyConnectionHandler::close() {
        std::lock_guard<std::mutex> lock{mutex};

        if (socket != nullptr) {
            socket->close();
            socket.reset(nullptr);
        }
    }

    void ProxyConnectionHandler::connect() throw(SocketException) {
        std::lock_guard<std::mutex> lock{mutex};

        if (socket == nullptr)
            throw SocketException("can't connect to a closed socket");
        socket->connect(hostName, port);
    }

    void ProxyConnectionHandler::sendData(std::string &lineData) {
        mutex.lock();
        try {
            socket->send(lineData.c_str(), lineData.length());
            mutex.unlock();
        } catch (SocketException e) {
            mutex.unlock();
            try {
                // try to close socket first and then reconnect
                close();
                socket.reset(new CommunicatingSocket());
                connect();
            } catch (SocketException &e) {
                throw e;
            }
        }
    }


}
