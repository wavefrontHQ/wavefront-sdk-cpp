#include "direct_ingestion/DirectIngesterService.h"

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

namespace wavefront {
    const static std::string CONTENT_TYPE = "application/octet-stream";
    // connection timeout, in ms
    const static int32_t TIMEOUT = 5000;

    DirectIngesterService::DirectIngesterService(std::string uri, std::string token) : uri(uri), token(token) {

    }

    cpr::Response DirectIngesterService::report(std::string format, std::list<std::string> targets) {
        cpr::Session session;
        // construct url
        std::string url = uri + "/report?f=" + format;
        session.SetUrl(cpr::Url{url});
        session.SetHeader(cpr::Header{{"Content-Type",     CONTENT_TYPE},
                                      {"Content-Encoding", "gzip"},
                                      {"Authorization",    "Bearer " + token}});
        session.SetTimeout(cpr::Timeout{TIMEOUT});
        session.SetBody(cpr::Body{getCompressedString(targets)});

        auto response = session.Post();
        return response;
    }

    std::string DirectIngesterService::getCompressedString(const std::list<std::string> &targets) {
        boost::iostreams::filtering_ostream compressingStream;
        std::string result = "";

        compressingStream.push(boost::iostreams::gzip_compressor());
        compressingStream.push(boost::iostreams::back_inserter(result));

        for (auto &stringToBeCompressed : targets)
            compressingStream << stringToBeCompressed;
        // flush and close
        boost::iostreams::flush(compressingStream);
        boost::iostreams::close(compressingStream);

        return result;
    }



}
