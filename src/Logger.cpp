#include "Logger.h"

const std::string Logger::log_format = "[%TimeStamp%] [ThreadID: %ThreadID%] [%Severity%]: %Message%";

void Logger::init_logger(bool std_out, std::string folder, unsigned int severity_level) {
    boost::log::register_simple_formatter_factory<boost::log::trivial::severity_level, char>("Severity");
    boost::log::add_common_attributes();

    boost::log::core::get()->set_filter (
        boost::log::trivial::severity >= severity_level
    );

    if(std_out == false && folder == "") {
        boost::log::core::get()->set_logging_enabled(false);
        return;
    }

    if(std_out)
        boost::log::add_console_log(std::cout, boost::log::keywords::format = log_format, boost::log::keywords::auto_flush = true);

    if(folder != "") {
        boost::log::add_file_log (
            boost::log::keywords::target = folder,
            boost::log::keywords::file_name = "Log_%N.log",
            boost::log::keywords::format = log_format
        );
    }
}
