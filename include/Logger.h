/*************************************************************************
 * This file is part of the BGP Extrapolator.
 *
 * Developed for the SIDR ROV Forecast.
 * This package includes software developed by the SIDR Project
 * (https://sidr.engr.uconn.edu/).
 * See the COPYRIGHT file at the top-level directory of this distribution
 * for details of code ownership.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 ************************************************************************/

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

//Add the channel attribute as a standard string
BOOST_LOG_ATTRIBUTE_KEYWORD(channel, "Channel", std::string)

/**
*   The logger class is meant to allow easy and simple logging around the project.
*   We would not like to pass around a Logger object across the entirety of the BGPExtrapolator.
*   Thus, the Logger class will store a single instance object that can be accessed statically as such:
*
*   #include "Logger.h"
*
*   Logger::setFolder
*
*   ....
*
*   Logger::getInstance().log("filename") << "Some Messege " << num;
*
*   OR
*
*   Logger::getInstance().log() << "Some Messege " << num;
*
*   The latter messege will be sent to the default log file, General.log
*
*   How it works:
*       This uses the Boost logging library in order to handle multiple named files. Specifically a multi-file sink is configured to name files based on
*           the provided channel name (a standard string). In addition, a channel logger is used to log with channels.
*       The use of channels is that it is a string type (good for naming) that can be filterd for files
*
*   Output:
*       Output is sent to the log file specified, with a timestamp and the messege.     
*
*   Important Notes:
*       The Logger will erase the log files stored in the log folder when the first instance is created. If 
*           a log holds important information move or copy the file elsewhere before running again.
*       Because of the above, make sure Logger::getInstance() is guaranteed to be called at least once.
*           The concequence would be log files from a previous run that someone may assume is from the current run (assuming the current run generated no logs).
*/
typedef boost::log::sinks::synchronous_sink<boost::log::sinks::text_multifile_backend> multifile_sink;

class Logger {
private:
    static std::string folder;
    static boost::shared_ptr<multifile_sink> multifile;

    boost::log::sources::channel_logger<> channel_lg;

    Logger();

public:
    /**
    *   Returns the single instance of the Logger.
    * 
    *   If this function has not been called before, then this will create the instance (concequently it will delete old log files)
    *   This should be guarenteed to be called in the start of the main. This is because if the function only gets called in an 
    *       if statement, then there is a chance old log files will live through a run of the extrapolator where they don't apply
    */
    static Logger getInstance();

    static void setFolder(std::string folder);

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
    public:
        boost::log::sources::channel_logger<> lg;
        std::string filename;
        std::stringstream stringStreamer;

    public:
        StreamBuff(boost::log::sources::channel_logger<> logger, const std::string file) : lg(logger), filename(file) {}
        StreamBuff(const StreamBuff &other) : lg(other.lg), filename(other.filename) {}

        ~StreamBuff();

        template<typename T>
        StreamBuff& operator<<(const T& output);

        template<typename T>
        StreamBuff& operator<<(const std::vector<T>& output);
    };

    /**
    *   Will send the output to the default log file "General.log"
    * 
    *   @return The stream buffer object that handles streaming the output to the default file
    */
    StreamBuff log();

    /**
    *   This will create the temporary object to stream the output to the log file specified
    *   
    *   @param The Filename for the log file that will be generated or added to if it already exists 
    *   @return The stream buffer object that handles streaming the output to the default file
    */
    StreamBuff log(std::string filename);
};

/* Don't move this to the cpp file, it will cause a compilation issue. Specifically:
*       undefined reference to `Logger::StreamBuff& Logger::StreamBuff::operator<<
*  However, having the same exact defintion here in the header compiles just fine.
*  It is a known problem and is being looked into
*/
template<typename T>
Logger::StreamBuff& Logger::StreamBuff::operator<<(const T& output) {
    if(Logger::folder != "")
        stringStreamer << output;//add to the string stream
    return *this;
}

template<typename T>
Logger::StreamBuff& Logger::StreamBuff::operator<<(const std::vector<T>& output) {
    if(Logger::folder != "") {
        // stringStreamer << output;//add to the string stream
        stringStreamer << "{ ";
        for(size_t i = 0; i < output.size(); i++) {
            if(i < output.size() - 1)
                stringStreamer << output.at(i) << ", ";
            else
                stringStreamer << output.at(i);
        }
        stringStreamer << " }";
    }
    return *this;
}

#endif