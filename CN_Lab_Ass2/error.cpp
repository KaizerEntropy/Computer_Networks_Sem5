#include "error.hpp"
#include <iostream>
#include <cstdlib>
#include <chrono>
#include <thread>

bool channel_transmit(Frame& f, double prob_loss, double prob_error, double prob_delay) {
    if ((rand() % 100) < (prob_loss * 100)) {
        std::cout << "[Channel] Frame " << (int)f.seq_no << " LOST.\n";
        return false;
    }

    if ((rand() % 100) < (prob_error * 100)) {
        std::cout << "[Channel] Frame " << (int)f.seq_no << " CORRUPTED.\n";
        f.fcs = f.fcs ^ 0xFFFFFFFF; // corrupt the FCS
    }
    
    // Introduce probabilistic transmission delay (20ms) which exceeds the 10ms socket timeout
    // to simulate premature timeouts in the network (timer < RTT)
    if ((rand() % 100) < (prob_delay * 100)) {
        std::cout << "[Channel] Frame " << (int)f.seq_no << " DELAYED (>10ms).\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    
    return true;
}
