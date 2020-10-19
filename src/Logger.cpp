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

#include "Logger.h"

std::string Logger::folder = "";
boost::shared_ptr<multifile_sink> Logger::multifile = boost::make_shared<multifile_sink>();

Logger::Logger() {
    std::cout << "Logger Created" << std::endl;

    /* 
    *   Remove any logs that already exist, and send the possible output to the void
    *   if statement to remove warning of not checking the return value
    *   an error generates if there are no files, but we don't really want to
    *   do anything in that case.
    */
    if(folder != "") {
        std::string rmCommand = "rm ";
        rmCommand.append(folder);
        rmCommand.append("*.log > /dev/null 2>&1");

        if(system(rmCommand.c_str())) {}
    }

    //Adds the time-stamp attribute
    boost::log::add_common_attributes();

    //sets up the multi-file sink based on the name of the channel
    multifile->locked_backend()->set_file_name_composer(
        boost::log::sinks::file::as_file_name_composer(
        boost::log::expressions::stream << folder << channel << ".log"));
    
    //Describes the format of the output files (this format applies to all files generated)
    //Format: [TimeStamp] Messege
    multifile->set_formatter(
        boost::log::expressions::stream << 
            "[" << boost::log::expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f %p") << "] " 
            << boost::log::expressions::smessage
    );

    boost::log::core::get()->add_sink(multifile);
}

void Logger::setFolder(std::string f) {
    folder = f;

    multifile->locked_backend()->set_file_name_composer(
        boost::log::sinks::file::as_file_name_composer(
        boost::log::expressions::stream << folder << channel << ".log"));

    if(folder != "") {
        std::string rmCommand = "rm ";
        rmCommand.append(folder);
        rmCommand.append("*.log > /dev/null 2>&1");

        if(system(rmCommand.c_str())) {}
    }
}

Logger Logger::getInstance() {
    static Logger instance;

    return instance;
}

Logger::StreamBuff::~StreamBuff() { 
    if(Logger::folder == "")
        return;

    std::string str = stringStreamer.str();
    if(!str.empty())//if there is something to output, make the boost call to log to the file
        BOOST_LOG_CHANNEL(lg, filename) << str;
}

Logger::StreamBuff Logger::log() {
    return log("General");
}

Logger::StreamBuff Logger::log(std::string filename) {
    return StreamBuff(getInstance().channel_lg, filename);
}