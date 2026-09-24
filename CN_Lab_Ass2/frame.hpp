#pragma once
#include <cstdint>
#include <vector>

#define MAX_PAYLOAD 1500
#define MIN_PAYLOAD 46

enum class FrameType : uint8_t {
    DATA = 0,
    ACK = 1,
    NAK = 2,
    SETUP = 3
};

struct Frame {
    uint8_t src_mac[6];
    uint8_t dest_mac[6];
    uint16_t length;
    uint8_t seq_no;
    FrameType type; 
    uint8_t ack_seq_no; 
    std::vector<uint8_t> payload;
    uint32_t fcs; // Frame Check Sequence

    std::vector<uint8_t> serialize() const;
    static Frame deserialize(const std::vector<uint8_t>& buffer);
    void calculate_fcs(int scheme); // 1 for checksum, 2 for CRC
    bool verify_fcs(int scheme) const;
};
