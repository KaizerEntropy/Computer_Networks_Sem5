#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>

enum class ChannelState {
    IDLE,
    BUSY,
    COLLISION
};

class Channel {
private:
    ChannelState state;
    std::mutex mtx;
    std::condition_variable cv;
    int active_transmissions;

public:
    Channel();
    
    // Check if the channel is idle
    bool is_idle();
    
    // Station starts transmitting
    bool start_transmission();
    
    // Station finishes transmitting
    void end_transmission();
    
    // Returns current state for collision detection
    ChannelState get_state();
};

#endif // CHANNEL_HPP
