#pragma once

#include <string>
#include <exception>         // For exception class

namespace wavefront {
/**
 *   Signals a problem with the execution of a socket call.
 */
    class SocketException : public std::exception {
    public:
        /**
         *   Construct a SocketException with a explanatory message.
         *   @param message explanatory message
         *   @param incSysMsg true if system message (from strerror(errno))
         *   should be postfixed to the user provided message
         */
        SocketException(const std::string &message, bool inclSysMsg = false) throw();

        /**
         *   Provided just to guarantee that no exceptions are thrown.
         */
        ~SocketException() throw();

        /**
         *   Get the exception message
         *   @return exception message
         */
        const char *what() const throw();

    private:
        std::string userMessage;  // Exception message
    };
}

