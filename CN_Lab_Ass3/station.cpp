#include "station.hpp"
#include <iostream>
#include <chrono>

using namespace std;

Station::Station(int id, Channel& channel, CSMAScheme scheme, int frames_to_send, double p_prob)
    : id(id), channel(channel), scheme(scheme), frames_to_send(frames_to_send), p_probability(p_prob) {
    rng.seed(std::chrono::system_clock::now().time_since_epoch().count() + id);
}

void Station::backoff(int attempt) {
    // Binary exponential backoff
    int max_slots = (1 << std::min(attempt, 10)) - 1;
    std::uniform_int_distribution<int> dist(0, max_slots);
    int wait_slots = dist(rng);
    
    // Simulate slot time (e.g., 2ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(wait_slots * 2));
}

void Station::carrier_sense() {
    std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
    std::uniform_int_distribution<int> random_wait(5, 15);
    
    while (true) {
        if (channel.is_idle()) {
            if (scheme == CSMAScheme::NON_PERSISTENT || scheme == CSMAScheme::ONE_PERSISTENT || scheme == CSMAScheme::CSMA_CD) {
                break; // Transmit immediately
            } else if (scheme == CSMAScheme::P_PERSISTENT) {
                if (prob_dist(rng) <= p_probability) {
                    break; // Transmit with probability p
                } else {
                    // Wait one time slot and check again
                    std::this_thread::sleep_for(std::chrono::milliseconds(2));
                }
            }
        } else {
            // Channel is BUSY
            if (scheme == CSMAScheme::NON_PERSISTENT) {
                // Wait a random amount of time before sensing again
                std::this_thread::sleep_for(std::chrono::milliseconds(random_wait(rng)));
            } else {
                // 1-persistent, p-persistent, CSMA/CD continuously sense
                // Yield thread to avoid 100% CPU usage on spinlock
                std::this_thread::yield();
            }
        }
    }
}

bool Station::transmit_frame() {
    channel.start_transmission();
    bool success = true;
    
    // Simulate transmission time (e.g., 10ms)
    // CSMA/CD actively detects collision during transmission
    for (int i = 0; i < 10; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (scheme == CSMAScheme::CSMA_CD && channel.get_state() == ChannelState::COLLISION) {
            success = false;
            break; // Abort transmission early
        }
    }
    
    // If not CSMA/CD, we only realize collision after full transmission time
    if (scheme != CSMAScheme::CSMA_CD && channel.get_state() == ChannelState::COLLISION) {
        success = false;
    }
    
    channel.end_transmission();
    return success;
}

void Station::run() {
    int frames_sent = 0;
    while (frames_sent < frames_to_send) {
        auto start_time = std::chrono::steady_clock::now();
        int attempts = 0;
        bool transmitted = false;
        
        while (!transmitted && attempts < 15) { // Max 15 attempts per frame
            carrier_sense();
            
            if (transmit_frame()) {
                transmitted = true;
                frames_sent++;
                stats.total_frames_sent++;
            } else {
                stats.total_collisions++;
                attempts++;
                
                if (scheme == CSMAScheme::CSMA_CD) {
                    // Send Jam signal
                    std::this_thread::sleep_for(std::chrono::milliseconds(2));
                }
                
                backoff(attempts);
            }
        }
        
        auto end_time = std::chrono::steady_clock::now();
        stats.total_delay_ms += std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    }
}

StationStats Station::get_stats() const {
    return stats;
}
