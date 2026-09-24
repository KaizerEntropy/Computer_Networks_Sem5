#include "crc.hpp"

CrcPoly getCrcPoly(int width) {
    if (width == 8) return {0xD5, 8};
    if (width == 10) return {0x233, 10};
    if (width == 16) return {0x8005, 16};
    if (width == 32) return {0x04C11DB7, 32};
    return {0, 0};
}

uint32_t compute_crc(const uint8_t *data, int len, CrcPoly p) {
    uint32_t mask;
    if (p.width == 32) {
        mask = 0xFFFFFFFFu;
    } else {
        mask = (1u << p.width) - 1u;
    }
    
    uint32_t topbit = 1u << (p.width - 1);
    uint32_t reg = 0;

    for (int i = 0; i < len * 8; i++) {
        int byte_idx = i / 8;
        int bit_idx = 7 - (i % 8);
        uint32_t bit = (data[byte_idx] >> bit_idx) & 1u;
        
        uint32_t carry = 0;
        if ((reg & topbit) != 0) {
            carry = 1;
        }
        
        reg = ((reg << 1) | bit) & mask;
        
        if (carry == 1) {
            reg = reg ^ p.poly;
        }
    }
    return reg;
}
