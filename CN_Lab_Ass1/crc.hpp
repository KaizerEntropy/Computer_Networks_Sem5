#ifndef CRC_HPP
#define CRC_HPP
#include <cstdint>
struct CrcPoly {
    uint32_t poly;
    int width;
};
CrcPoly getCrcPoly(int width);
uint32_t compute_crc(const uint8_t *data, int len, CrcPoly p);
#endif
