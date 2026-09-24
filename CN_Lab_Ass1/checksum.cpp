#include "checksum.hpp"

uint16_t compute_checksum(const uint8_t *data, int len) {
    uint32_t sum = 0;
    int i = 0;
    
    // Loop through 2 bytes at a time
    while (i + 1 < len) {
        uint16_t word = (uint16_t)((data[i] << 8) | data[i + 1]);
        sum = sum + word;
        
        // Handle end-around carry
        if (sum > 0xFFFF) {
            sum = (sum & 0xFFFF) + 1;
        }
        i = i + 2;
    }
    
    // If there is an odd byte left over
    if (i < len) {
        sum = sum + (uint16_t)(data[i] << 8);
        if (sum > 0xFFFF) {
            sum = (sum & 0xFFFF) + 1;
        }
    }
    
    // 1's complement
    return (uint16_t)(~sum & 0xFFFF);
}
