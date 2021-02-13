#ifndef PROGRESS_BAR_H
#define PROGRESS_BAR_H
#include <cstdint>
#include <string>
#include <chrono>
#include "Logger.h"

class ProgressBar {
public:
    /**
     * @param total_iterations -> The number of iterations before the progress bar reaches 100%
     */
    ProgressBar(uint32_t total_iterations);

    ~ProgressBar();

    // Make the object non-copyable
    ProgressBar(const ProgressBar& p) = delete;
    ProgressBar& operator=(const ProgressBar& p) = delete;

    // Increment the progress bar
    void increment();

    /**
     * Prints a message above the progress bar
     * 
     * @param message -> A message to print
     * @param severity -> Priority for the logger (ex: boost::log::trivial::info)
     */
    void print_new_line(std::string message, boost::log::trivial::severity_level severity);
    void end_progress_bar();
private:
    uint32_t progress;
    uint32_t total_iterations;
    uint32_t current_iterations;
    uint32_t width;
    uint32_t window_width;
    double seconds_remaining;
    bool finished;
    std::chrono::time_point<std::chrono::high_resolution_clock> start;

    /**
     * Forms the progress bar string
     * 
     * @param force_full_bar -> If the number of bars remains the same, the function only returns time remaining. "True" to override that behavior
     */
    std::string generate_progress_bar(bool force_full_bar = false);

    // Generate a string with time remaining
    std::string format_seconds();

    // Calculates the current percentage of the progress bar 
    uint32_t calculate_percentage();
};   

#endif