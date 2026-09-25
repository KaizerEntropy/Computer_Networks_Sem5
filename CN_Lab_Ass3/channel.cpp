#include "channel.hpp"

Channel::Channel() : state(ChannelState::IDLE), active_transmissions(0) {}

bool Channel::is_idle() {
    std::lock_guard<std::mutex> lock(mtx);
    return state == ChannelState::IDLE;
}

bool Channel::start_transmission() {
    std::lock_guard<std::mutex> lock(mtx);
    active_transmissions++;
    
    if (active_transmissions == 1) {
        state = ChannelState::BUSY;
        return true; // Successfully claimed channel
    } else {
        state = ChannelState::COLLISION;
        return false; // Collision occurred
    }
}

void Channel::end_transmission() {
    std::lock_guard<std::mutex> lock(mtx);
    active_transmissions--;
    
    if (active_transmissions == 0) {
        state = ChannelState::IDLE;
    } else if (active_transmissions == 1) {
        state = ChannelState::BUSY;
    } else {
        state = ChannelState::COLLISION;
    }
}

ChannelState Channel::get_state() {
    std::lock_guard<std::mutex> lock(mtx);
    return state;
}
