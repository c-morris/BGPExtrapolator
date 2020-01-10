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

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;

//Add the channel attribute as a standard string
BOOST_LOG_ATTRIBUTE_KEYWORD(channel, "Channel", std::string)

/**
*   The logger class is meant to allow easy and simple logging around the project.
*   We would not like to pass around a Logger object across the entirety of the BGPExtrapolator.
*   Thus, the Logger class will store a single instance object that can be accessed statically as such:
*
*   #include "Logger.h"
*
*   ....
*
*   Logger::getInstance().log("filename") << "Some Messege " << num;
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
public:
    boost::log::sources::channel_logger<> channel_lg;

    Logger();

    /**
    *   Returns the single instance of the Logger.
    * 
    *   If this function has not been called before, then this will create the instance (concequently it will delete old log files)
    *   This should be guarenteed to be called in the start of the main. This is because if the function only gets called in an 
    *       if statement, then there is a chance old log files will live through a run of the extrapolator where they don't apply
    */
    static Logger getInstance() {
        static Logger instance;

        return instance;
    }

    /**
    *   This is an interesting solution to overloading the << operator.
    *   The main issue is that every time the boost logger logs, it will create a new line. This is not in our favor 
    *       because Logger::getInstance().log("file") << "Some text" << "More text"; will output on multiple lines. 
    *       So to counter this, a temporary object will be created. That object will override <<, but instead of 
    *       logging the text instantly, it will store the result in a string stream, to build the output.
    *   
    *   Moreover, with the above true, there is the matter of recognizing when the end of the line is.
    *       The object only knows to store the partial output, it doesn't know when to log it to the file.
    *       The counter here is to create the streaming object at the log() function call. That way, at the end of 
    *       the line, the object will be deconstructed. It is in the deconstructor that the output will be logged
    *       to the corresponding file.
    */
    class StreamBuff {
    private:
        boost::log::sources::channel_logger<> lg;
        std::string filename;
        std::stringstream stringStreamer;
    public:
        StreamBuff(boost::log::sources::channel_logger<> logger, const std::string file) : lg(logger), filename(file) {}
        StreamBuff(const StreamBuff &other) : lg(other.lg), filename(other.filename) {}

        ~StreamBuff() { 
            std::string str = stringStreamer.str();
            if(!str.empty())//if there is something to output, make the boost call to log to the file
                BOOST_LOG_CHANNEL(lg, filename) << str;
        }

        template<typename T>
        StreamBuff& operator<<(const T& output) {
            stringStreamer << output;//add to the string stream
            return *this;
        }
    };

    /**
    *   Will send the output to the default log file "General.log"
    * 
    *   @return The stream buffer object that handles streaming the output to the default file
    */
    StreamBuff log() {
        return log("General");
    }

    /**
    *   This will create the temporary object to stream the output to the log file specified
    *   
    *   @param The Filename for the log file that will be generated or added to if it already exists 
    *   @return The stream buffer object that handles streaming the output to the default file
    */
    StreamBuff log(std::string filename) {
        return StreamBuff(getInstance().channel_lg, filename);
    }
};
#endif