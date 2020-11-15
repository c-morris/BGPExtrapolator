#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include <fstream>

class Logger {
private:
    static const std::string log_format;

public:
    /**
     * This initializes the Boost logging with the output format we want. The logs to the files and the console look the exact same.
     * 
     * ONLY CALL THIS ONCE. BEFORE ANY LOGGING OCCURS.
     * 
     * Outputting the logs more than once is fine. Boost will call them Log_0.log, Log_1.log. It will just keep counting up.
     * 
     * This is the enum for severity levels from boost:
     *  enum severity_level {
     *      trace,   // 0
     *      debug,   // 1
     *      info,    // 2
     *      warning, // 3
     *      error,   // 4
     *      fatal    // 5
     *  };
     * 
     * By providing the severity_level, it will filter to keep that level and everything above it.
     * So a severity level of 2 (info) would make it output info, warning, error, and fatal messeges. The rest would not print or be written to the file.
     * 
     * Logging can also be totally disabled with: boost::log::core::get()->set_logging_enabled(false);
     * When the log folder is "" and the console outupt parameter is false, logging is completely disabled. Do not turn it back on at any point after.
     * 
     * After this, logging can be done like this:
     *      BOOST_LOG_TRIVIAL(trace) << "This is a trace severity message";
     * Place in the severity level (from the enum above) where trace is.
     * 
     * @param std_out -> A boolean that expresses whether logging should go out to the console
     * @param folder -> The folder to put the log files into. "" for no file output.
     * @param log_severity -> The severity to include and everything above it
     */
    static void init_logger(bool std_out, std::string folder, unsigned int severity_level);
};