#ifndef ERROR_HPP
#define ERROR_HPP
#include <cstdint>
#include <string>

enum ErrorType {
    ERR_NONE = 0,
    ERR_SINGLE_BIT = 1,
    ERR_TWO_ISOLATED = 2,
    ERR_ODD_BITS = 3,
    ERR_BURST = 4,
    ERR_CHECKSUM_MISS = 5,
    ERR_CRC_MISS = 6
};

const int ROUND_ROBIN_CYCLE = 7;
std::string errorTypeName(int e);

struct InjectionResult {
    bool error_injected;
    std::string description;
};

InjectionResult inject_error(uint8_t *frame, int frame_bytes, int error_type, int scheme);

#endif
