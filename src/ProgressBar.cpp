#include "ProgressBar.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <sys/ioctl.h>

ProgressBar::ProgressBar(uint32_t total_iterations) {
    if (total_iterations < 2) {
        total_iterations = 2;
    }
    this->total_iterations = total_iterations;
    current_iterations = 0;
    progress = 0;
    finished = false;

    // Get the number of columns of the console (width)
    struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);
    window_width = w.ws_col;

    if (window_width < 10) {
        // Not enough space for the progress bar
        return;
    }

    // Set width of the actual bar to width minus 8 
    // (2 spaces for padding, 2 spaces for brackets around the bar, 
    //  3 spaces for the percentage number, 1 space for thepercentage sign)
    width = window_width - 8;
    
    std::cout << "Width: " << width << std::endl;

    start = std::chrono::high_resolution_clock::now();

    std::cout << generate_progress_bar() << "\r" << std::flush;
    //std::cout << "\x1b[2A" << std::flush;
}

ProgressBar::~ProgressBar() {
    end_progress_bar();
}

void ProgressBar::operator++() {
    if (finished) {
        // Not sure if that is a good idea
        throw std::runtime_error("The progress bar has reached the end, can't increment it anymore");
    }

    // Increment the number of iterations
    ++current_iterations; // = std::min(current_iterations+1, total_iterations);

    if (current_iterations % 25 == 0) {
        finish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = finish - start;
        double time_remaining = (duration.count() / current_iterations) * (total_iterations - current_iterations);
        std::cout << duration.count() << std::endl;
        std::cout << time_remaining << std::endl;
        std::cout << format_seconds((int) time_remaining) << std::endl;

        //start = std::chrono::high_resolution_clock::now();
    }

    std::string progress_bar = generate_progress_bar();

    // Only redraw the progress bar if it updated
    if (progress_bar != "") {
        /**
        *  Set width of the output to the entire width of the window
        *  Make sure that the output is alligned to the left
        *  Return carriage to the beginning of the line after printing the bar
        */ 
        std::cout << std::left << std::setw(window_width) << progress_bar << "\r" << std::flush;
    }

    // BOOST_LOG_TRIVIAL(info) << generate_progress_bar(percentage) << "\r";
    // BOOST_LOG_TRIVIAL(info) << "\x1b[2A";

    if (current_iterations == total_iterations) {
        end_progress_bar();
    }
}

void ProgressBar::print_new_line(std::string message, boost::enum::sever severity) {
    if (finished) {
        throw std::runtime_error("The progress bar has reached the end, can't increment it anymore");
    }

    // Calculate the length of the logger output
    // When filled with falues, this is 65 chars long minus length of severity:
    // [%TimeStamp%] [ThreadID: %ThreadID%] [%Severity%]: 
    uint32_t output_length = message.size() + severity.size() + 65;

    /**
    *  Set width of the output to the entire width of the window minus the output length 
    *  (not sure why, but if I set it to just window_width it takes up two lines)
    *  Make sure that the output is alligned to the left
    */ 
    BOOST_LOG_TRIVIAL(info) << std::left << std::setw(window_width - output_length) << message; //<< "\r" << std::left << std::setw(width+73) << message << "\n";

    // Output the progress bar and return carriage to the beginning of the line after printing the bar
    std::cout << generate_progress_bar() << "\r" << std::flush;    
}

void ProgressBar::end_progress_bar() {
    if (!finished) {
        std::cout << std::string(2, '\n') << std::flush;
        finished = true;
    }
}

std::string ProgressBar::generate_progress_bar() {
    uint32_t percentage = calculate_percentage();

    // Progress = how many chars in the bar are filled
    // Multiply percentage by width of the bar and divide by 100
    const uint32_t new_progress = (int) std::round((double) percentage * width / 100);

    // Don't update the bar if progress is the same
    if (new_progress == progress) {
        return "";
    } else {
        progress = new_progress;
    }

    std::ostringstream bar_str;
    // Add percentage to the stream and allign it to the right
    bar_str << " " << std::setw(3) << std::right << percentage << "%";
    // Create a string with an empty progress bar
    std::string bar("[" + std::string(width, ' ') + "]");

    // Fill the progress bar 
    bar.replace(1, progress, std::string(progress, '|'));

    // Append the progress bar to the stringstream
    bar_str << bar;
    return bar_str.str();
}

uint32_t ProgressBar::calculate_percentage() {
    // Divide current number of iterations by the amount of iterations needed for 100%
    // and multiply by 100
    return (int)std::round(current_iterations * 100 / total_iterations);
}

std::string ProgressBar::format_seconds(uint32_t seconds) {
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;

    std::ostringstream output;
    output << std::to_string(hours) << ":" << minutes % 60 << ":" << seconds % 60;

    return output.str();
}