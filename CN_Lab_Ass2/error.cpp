#include "error.hpp"
#include <iostream>
#include <cstdlib>

bool channel_transmit(Frame& f, double prob_loss, double prob_error) {
    if ((rand() % 100) < (prob_loss * 100)) {
        std::cout << "[Channel] Frame " << (int)f.seq_no << " LOST.\n";
        return false;
    }

    if ((rand() % 100) < (prob_error * 100)) {
        std::cout << "[Channel] Frame " << (int)f.seq_no << " CORRUPTED.\n";
        f.fcs = f.fcs ^ 0xFFFFFFFF; // corrupt the FCS
    }
    return true;
}
