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

//Add the channel attribute
BOOST_LOG_ATTRIBUTE_KEYWORD(channel, "Channel", std::string)

/*
*   The logger class is meant to allow easy and simple logging around the project.
*   We would not like to pass around a Logger object across the entirety of the BGPExtrapolator.
*   Thus, the Logger class will store a single instance object that can be accessed statically as such:
*
*   #include "Logger.h"
*
*   ....
*
*   Logger::getInstance().setFilename("Some Name");
*   Logger::getInstance() << "Some Messege " << num;
*
*   How it works:
*       This uses the Boost logging library in order to handle multiple named files. Specifically a multi-file sink is configured to name files based on
*           the provided channel name (a standard string). In addition, a channel logger is used to log with channels.
*       The use of channels is that it is a string type (good for naming) that can be filterd for files
*
*   Important Notes:
*       The Logger will erase the log files stored in the log folder when the first instance is created. If 
*           a log holds important information move or copy the file elsewhere before running again.
*       Because of the above, make sure Logger::getInstance() is guaranteed to be called at least once.
*           The concequence would be log files from a previous run that someone may assume is from the current run (assuming the current run generated no logs).
*/
class Logger {
private:
    boost::log::sources::channel_logger<> channel_lg;
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

/*
*   The idea here is to build on the stream provided by Boost logging
*
*   Intended use: Logging::getInstance() << "Some messege"
*   This messege will be sent to the log file stored in 
*/
template<class T>
Logger& operator<<(Logger& log, const T& output) {
    //Specify the channel name as the filename (the sink interprets the channel name as the filename)
    BOOST_LOG_CHANNEL(log.channel_lg, log.filename) << output;

    return log;
}
#endif
