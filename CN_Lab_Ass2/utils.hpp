#pragma once
#include <iostream>
#include <vector>

struct SimulationStats {
    int total_frames;
    int total_transmissions;
    int total_timeouts;
    double elapsed_time_ms;
    double prob_loss;
    double prob_error;
    double prob_delay;
    int protocol; // 1=SW, 2=GBN, 3=SR
    
    // For RTT calculations
    std::vector<double> rtt_samples; 
    
    SimulationStats() : total_frames(0), total_transmissions(0), total_timeouts(0), 
                        elapsed_time_ms(0), prob_loss(0), prob_error(0), prob_delay(0), protocol(1) {}
};

void print_visualization(const SimulationStats& stats);
