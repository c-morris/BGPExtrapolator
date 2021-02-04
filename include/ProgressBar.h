#ifndef PROGRESS_BAR_H
#define PROGRESS_BAR_H
#include <cstdint>
#include <string>
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

    // Overload prefix increment, used to iterate through the progress bar
    void operator++();

    /**
     * Prints a message above the progress bar
     * 
     * @param message -> A message to print
     * @param priority -> Message priority (passed into the logger)
     */
    void print_new_line(std::string message, std::string severity);
    void end_progress_bar();
private:
    uint32_t progress;
    uint32_t total_iterations;
    uint32_t current_iterations;
    uint32_t width;
    uint32_t window_width;
    bool finished;
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
    std::chrono::time_point<std::chrono::high_resolution_clock> finish;

    /**
     * Forms the progress bar string
     * 
     * @param percentage -> Current percentage of the progress bar
     */
    std::string generate_progress_bar();

    std::string format_seconds(uint32_t seconds);

    // Calculates the current percentage of the progress bar 
    uint32_t calculate_percentage();
};   

#endif