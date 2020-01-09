#ifndef LOGGER_H
#define LOGGER_H

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/log/core.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sources/channel_logger.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/support/date_time.hpp>

#include <iostream>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;

BOOST_LOG_ATTRIBUTE_KEYWORD(channel, "Channel", std::string)

class Logger {
private:
    boost::log::sources::channel_logger<> channel_lg{boost::log::keywords::channel = "Main"};
    std::string filename;

    Logger();
public:
    static Logger& getInstance() {
        static Logger instance;

        return instance;
    }

    void setFilename(std::string filename);

    template<class T>
    friend Logger& operator<<(Logger& logger, const T& output);
};
#endif