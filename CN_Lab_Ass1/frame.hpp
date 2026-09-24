#ifndef FRAME_HPP
#define FRAME_HPP
#include <cstdint>
#include <string>

const int DEST_LEN = 6;
const int SRC_LEN = 6;
const int LEN_FIELD = 2;
const int HEADER_LEN = DEST_LEN + SRC_LEN + LEN_FIELD;
const int PAYLOAD_SIZE = 110;
const int FCS_SIZE = 4;
const int FRAME_SIZE = HEADER_LEN + PAYLOAD_SIZE + FCS_SIZE;
const int FCS_COVER_LEN = HEADER_LEN + PAYLOAD_SIZE;

enum DetectionScheme { CHECKSUM16 = 1, CRC8 = 2, CRC10 = 3, CRC16 = 4, CRC32 = 5 };

struct SchemeInfo {
    std::string name;
    int width_bits;
};

SchemeInfo getSchemeInfo(int code);

enum FileType { FILE_TEXT = 0, FILE_BITS = 1 };
std::string fileTypeName(int ft);

struct Frame {
    uint8_t dest[DEST_LEN];
    uint8_t src[SRC_LEN];
    uint16_t length;
    uint8_t payload[PAYLOAD_SIZE];
    uint8_t fcs[FCS_SIZE];
};

void frame_to_bytes(const Frame &f, uint8_t out[FRAME_SIZE]);
Frame frame_from_bytes(const uint8_t in[FRAME_SIZE]);

#endif
