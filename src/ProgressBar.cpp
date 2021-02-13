#include "ProgressBar.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <sys/ioctl.h>

ProgressBar::ProgressBar(uint32_t total_iterations) {
    if (total_iterations < 2) {
        // Too little iterations
        BOOST_LOG_TRIVIAL(warning) << "Too little iterations for the progress bar";
        return;
    }
    this->total_iterations = total_iterations;
    current_iterations = 0;
    progress = 0;
    seconds_remaining = 0;
    finished = false;

    // Get the number of columns of the console (width)
    struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);
    window_width = w.ws_col;
    if (window_width < 24) {
        // Not enough space for the progress bar
        BOOST_LOG_TRIVIAL(warning) << "The window is too narrow for the progress bar";
        return;
    }
    // Set width of the actual bar to width minus 22 
    // (2 spaces for padding, 2 spaces for brackets around the bar, 
    //  3 spaces for the percentage number, 1 space for thepercentage sign,
    //  14 spaces for the time remaining)
    width = window_width - 22;
    
    // Start the clock, it will be used for time estimation
    start = std::chrono::high_resolution_clock::now();

    // Draw the progress bar
    std::cout << generate_progress_bar(true) << "\r" << std::flush;
}

ProgressBar::~ProgressBar() {
    end_progress_bar();
}

void ProgressBar::increment() {
    if (finished) {
        BOOST_LOG_TRIVIAL(error) << "The progress bar has reached the end, can't increment it anymore";
        return;
    }

    if (current_iterations >= total_iterations) {
        return;
    }

    // Increment the number of iterations
    ++current_iterations;

    // Get the time elapsed since timestamp in the start variable
    std::chrono::duration<double> duration = std::chrono::high_resolution_clock::now() - start;
    double duration_number = duration.count();
    // Recalculate estimated time until completion
    seconds_remaining = (duration_number / current_iterations) * (total_iterations - current_iterations);
    
    // Get a string with estimated time and, if it changed, a new progress bar 
    std::string progress_bar = generate_progress_bar();
    if (progress_bar.size() == 19 && duration_number >= 5) { 
        /**
        *  If only the estimated time updated, but the bar stayed the same, 
        *  set width of the output to the width of the estimated time and percentage
        *  Make sure that the output is alligned to the left
        *  Return carriage to the beginning of the line after printing the bar
        */ 
        std::cout << std::left << std::setw(19) << progress_bar << "\r" << std::flush;
    } else if (progress_bar.size() > 19) {
        /**
        *  Set width of the output to the entire width of the window
        *  Make sure that the output is alligned to the left
        *  Return carriage to the beginning of the line after printing the bar
        */ 
        std::cout << std::left << std::setw(window_width) << progress_bar << "\r" << std::flush;
    }
}

void ProgressBar::print_new_line(std::string message, boost::log::trivial::severity_level severity) {
    // Clear the progress bar
    // It doesn't work properly without the empty string
    std::cout << std::setw(window_width) << "" << "\r" << std::flush;

    /**
    *  Output the message
    *  I had to replace BOOST_LOG_TRIVIAL with its macros because I didn't find another way
    *  to pass the severity level without using switch
    */ 
    BOOST_LOG_STREAM_WITH_PARAMS(::boost::log::trivial::logger::get(),(::boost::log::keywords::severity = severity))\
    << std::left << message;

    // Output the progress bar and return carriage to the beginning of the line after printing the bar
    // Force generate_progress_bar to redraw the bar
    std::cout << generate_progress_bar(true) << "\r" << std::flush;   
}

void ProgressBar::end_progress_bar() {
    if (!finished) {
        std::cout << std::string(2, '\n') << std::flush;
        finished = true;
    }
}

std::string ProgressBar::generate_progress_bar(bool force_full_bar) {
    uint32_t percentage = calculate_percentage();
    // Progress = how many chars in the bar are filled
    // Multiply percentage by width of the bar and divide by 100
    const uint32_t new_progress = (int) std::round((double) percentage * width / 100);

    std::string time_remaining = format_seconds();
    std::ostringstream bar_str;
    // Add percentage to the stream and allign it to the right and
    // add time to the stream and allign it to the left
    bar_str << " " << std::setw(3) << std::right << percentage << "%, " << std::setw(11) << std::left << time_remaining << " ";

    // Only update the bar if progress changed
    if (new_progress != progress || force_full_bar) {
        progress = new_progress;

        // Create a string with an empty progress bar
        std::string bar("[" + std::string(width , ' ') + "]");
        // Fill the progress bar 
        bar.replace(1, progress, std::string(progress, '|'));
        // Append the progress bar to the stringstream
        bar_str << bar;
    }

    return bar_str.str();
}

uint32_t ProgressBar::calculate_percentage() {
    // Divide current number of iterations by the amount of iterations needed for 100%
    // and multiply by 100
    return current_iterations * 100 / total_iterations;
}

std::string ProgressBar::format_seconds() {
    uint32_t seconds = (int) seconds_remaining;
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;

    std::ostringstream output;
    output << std::to_string(hours) << "H:" << minutes % 60 << "M:" << seconds % 60 << "S";

    return output.str();
}