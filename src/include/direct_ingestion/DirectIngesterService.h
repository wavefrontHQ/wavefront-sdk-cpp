#pragma once

#include <string>
#include <list>
#include <cpr/cpr.h>

namespace wavefront {
    /**
    *  DataIngester service that reports entities to Wavefront
    *
    *  @author Mengran Wang (mengranw@vmware.com)
    */
    class DirectIngesterService {
    public:
        DirectIngesterService(std::string url, std::string token);

        /**
        * The API for reporting points directly to a Wavefront server.
        *
        * @param format wavefront supported format see @constant.cpp
        * @param targets list of data points to be reported
        */
        cpr::Response report(std::string format, std::list<std::string> targets);

    private:
        std::string getCompressedString(const std::list<std::string> &targets);

        std::string uri;
        std::string token;
    };
}