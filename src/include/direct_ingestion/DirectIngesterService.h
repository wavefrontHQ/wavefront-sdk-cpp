#pragma once

#include <string>
#include <list>
#include <cpr/cpr.h>
namespace wavefront{
    class DirectIngesterService{
    public:
        DirectIngesterService(std::string url, std::string token);

        cpr::Response report(std::string format, std::list<std::string> targets);
    private:
        std::string getCompressedString(const std::list<std::string> &targets);
        std::string uri;
        std::string token;
    };
}