#include "checksum.hpp"
#include <cstddef>

uint16_t calculate_checksum(const std::vector<uint8_t>& data) {
    uint32_t sum = 0;
    for (size_t i = 0; i < data.size(); i += 2) {
        uint16_t word = (data[i] << 8);
        if (i + 1 < data.size()) {
            word |= data[i+1];
        }
        sum += word;
        if (sum > 0xFFFF) {
            sum = (sum & 0xFFFF) + 1;
        }
    }
    return ~sum & 0xFFFF;
}
