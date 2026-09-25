#ifndef STATION_HPP
#define STATION_HPP

#include "channel.hpp"
#include <random>
#include <chrono>
#include <thread>
#include <atomic>

enum class CSMAScheme {
    NON_PERSISTENT,
    ONE_PERSISTENT,
    P_PERSISTENT,
    CSMA_CD
};

struct StationStats {
    int total_frames_sent = 0;
    int total_collisions = 0;
    double total_delay_ms = 0;
};

class Station {
private:
    int id;
    Channel& channel;
    CSMAScheme scheme;
    double p_probability; // for p-persistent
    int frames_to_send;
    
    StationStats stats;
    std::mt19937 rng;
    
    // Backoff mechanism
    void backoff(int attempt);
    
    // Wait for channel to be idle based on scheme
    void carrier_sense();
    
    // Transmission logic
    bool transmit_frame();

public:
    Station(int id, Channel& channel, CSMAScheme scheme, int frames_to_send, double p_prob = 0.5);
    
    // Thread runner
    void run();
    
    StationStats get_stats() const;
};

#endif // STATION_HPP
