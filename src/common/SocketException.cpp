#include "common/SocketException.h"
#include <errno.h>             // For errno

namespace wavefront {
    SocketException::SocketException(const std::string &message, bool inclSysMsg)
    throw() : userMessage(message) {
        if (inclSysMsg) {
            userMessage.append(": ");
            userMessage.append(strerror(errno));
        }
    }

    SocketException::~SocketException() throw() {
    }

    const char *SocketException::what() const throw() {
        return userMessage.c_str();
    }
}
