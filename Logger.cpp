#include "Logger.h"

Logger::Logger() {
    if(system("rm ./Logs/* > /dev/null 2>&1")) {

    }

    boost::log::add_common_attributes();

    typedef boost::log::sinks::synchronous_sink<boost::log::sinks::text_multifile_backend> multifile_sink;
    boost::shared_ptr<multifile_sink> multifile = boost::make_shared<multifile_sink>();

    multifile->locked_backend()->set_file_name_composer(
        boost::log::sinks::file::as_file_name_composer(
        boost::log::expressions::stream << "Logs/" << channel << ".log"));
  
    multifile->set_formatter(
        boost::log::expressions::stream << 
            "[" << boost::log::expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f %p") << "] " 
            << boost::log::expressions::smessage
    );

    boost::log::core::get()->add_sink(multifile);
}

void Logger::setFilename(std::string filename) {
    this->filename = filename;
}

template<class T>
Logger& operator<<(Logger& log, const T& output) {
    BOOST_LOG_CHANNEL(log.channel_lg, log.filename) << output;

    return log;
}

// int main() {
//     Logger::getInstance().setFilename("Loops");
//     Logger::getInstance() << "Loop Loop Loop Loop Loop Loop Loop Loop Loop Loop Loop Loop";

//     Logger::getInstance().setFilename("Breaks");
//     Logger::getInstance() << "Breaky breaky break";
// }