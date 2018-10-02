#pragma once

#include <mutex>
#include "../common/Socket.h"

namespace wavefront {
    /**
    * Connection Handler class for sending data to a Wavefront proxy listening on a given port.
    *
    * @author Mengran Wang (mengranw@vmware.com)
    */
    class ProxyConnectionHandler {
    public:
        ProxyConnectionHandler(std::string &hostName, unsigned short port);

        ~ProxyConnectionHandler();

        void close();

        void connect() throw(SocketException);

        /**
        * Sends the given data to the WavefrontProxyClient proxy.
        *
        * @param lineData line data in a WavefrontProxyClient supported format
        * @throws Exception If there was failure sending the data
        */
        void sendData(std::string &lineData) throw(SocketException);

        inline int getFailureCount() {
            return failures.load();
        }

        inline int incrementFailureCount() {
            return failures.fetch_add(1);
        }

        inline const std::string getDefaultHost() {
            return socket->getLocalAddress();
        }

    private:
        std::unique_ptr<CommunicatingSocket> socket = nullptr;
        std::mutex mutex;
        std::string &hostName;
        unsigned short port;

        std::atomic<int> failures;
    };
}