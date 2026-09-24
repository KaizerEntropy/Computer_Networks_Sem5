#include "frame.hpp"
#include <cstring>

SchemeInfo getSchemeInfo(int code) {
    if (code == 1) return {"16-bit Checksum", 16};
    if (code == 2) return {"CRC-8", 8};
    if (code == 3) return {"CRC-10", 10};
    if (code == 4) return {"CRC-16", 16};
    if (code == 5) return {"CRC-32 (IEEE 802.3)", 32};
    return {"UNKNOWN", 0};
}

std::string fileTypeName(int ft) {
    if (ft == FILE_BITS) {
        return "Binary-bits file (.bits)";
    } else {
        return "Text file";
    }
}

void frame_to_bytes(const Frame &f, uint8_t out[FRAME_SIZE]) {
    int off = 0;
    
    // Copy Destination MAC
    for(int i=0; i<DEST_LEN; i++) out[off + i] = f.dest[i];
    off += DEST_LEN;
    
    // Copy Source MAC
    for(int i=0; i<SRC_LEN; i++) out[off + i] = f.src[i];
    off += SRC_LEN;
    
    // Copy Length
    out[off] = (uint8_t)((f.length >> 8) & 0xFF);
    out[off+1] = (uint8_t)(f.length & 0xFF); 
    off += LEN_FIELD;
    
    // Copy Payload
    for(int i=0; i<PAYLOAD_SIZE; i++) out[off + i] = f.payload[i];
    off += PAYLOAD_SIZE;
    
    // Copy FCS
    for(int i=0; i<FCS_SIZE; i++) out[off + i] = f.fcs[i];
}

Frame frame_from_bytes(const uint8_t in[FRAME_SIZE]) {
    Frame f;
    int off = 0;
    
    for(int i=0; i<DEST_LEN; i++) f.dest[i] = in[off + i];
    off += DEST_LEN;
    
    for(int i=0; i<SRC_LEN; i++) f.src[i] = in[off + i];
    off += SRC_LEN;
    
    f.length = (uint16_t)((in[off] << 8) | in[off+1]); 
    off += LEN_FIELD;
    
    for(int i=0; i<PAYLOAD_SIZE; i++) f.payload[i] = in[off + i];
    off += PAYLOAD_SIZE;
    
    for(int i=0; i<FCS_SIZE; i++) f.fcs[i] = in[off + i];
    
    return f;
}
