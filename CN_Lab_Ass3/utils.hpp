#pragma once
#include <cstdint>
#include <netinet/in.h>

enum class MsgType : uint8_t {
    SENSE = 1,
    SENSE_REPLY_IDLE = 2,
    SENSE_REPLY_BUSY = 3,
    TRANSMIT = 4,
    SUCCESS = 5,
    COLLISION = 6
};

struct Message {
    MsgType type;
    int station_id;
    int data; // Can be frame number
};

struct AddrComparator {
    bool operator()(const struct sockaddr_in& a, const struct sockaddr_in& b) const {
        if (a.sin_addr.s_addr != b.sin_addr.s_addr) return a.sin_addr.s_addr < b.sin_addr.s_addr;
        return a.sin_port < b.sin_port;
    }
};
