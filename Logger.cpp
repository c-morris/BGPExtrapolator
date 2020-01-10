#include "Logger.h"

Logger::Logger() {
    //Remove any logs that already exist, and send the possible output to the void
    if(system("rm ./Logs/* > /dev/null 2>&1")) {

    }

    //Adds the time-stamp attribute
    boost::log::add_common_attributes();

    typedef boost::log::sinks::synchronous_sink<boost::log::sinks::text_multifile_backend> multifile_sink;
    boost::shared_ptr<multifile_sink> multifile = boost::make_shared<multifile_sink>();

    //sets up the multi-file sink based on the name of the channel
    multifile->locked_backend()->set_file_name_composer(
        boost::log::sinks::file::as_file_name_composer(
        boost::log::expressions::stream << "Logs/" << channel << ".log"));
  
    //Describes the format of the output files (this format applies to all files generated)
    //Format: [TimeStamp] Messege
    multifile->set_formatter(
        boost::log::expressions::stream << 
            "[" << boost::log::expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f %p") << "] " 
            << boost::log::expressions::smessage
    );

    boost::log::core::get()->add_sink(multifile);
}
