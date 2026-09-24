#include "frame.hpp"
#include "checksum.hpp"
#include "crc.hpp"
#include <cstring>
#include <arpa/inet.h>

std::vector<uint8_t> Frame::serialize() const {
    std::vector<uint8_t> buffer;
    buffer.insert(buffer.end(), src_mac, src_mac + 6);
    buffer.insert(buffer.end(), dest_mac, dest_mac + 6);
    
    buffer.push_back((length >> 8) & 0xFF);
    buffer.push_back(length & 0xFF);
    
    buffer.push_back(seq_no);
    buffer.push_back(static_cast<uint8_t>(type));
    buffer.push_back(ack_seq_no);
    
    buffer.insert(buffer.end(), payload.begin(), payload.end());
    
    buffer.push_back((fcs >> 24) & 0xFF);
    buffer.push_back((fcs >> 16) & 0xFF);
    buffer.push_back((fcs >> 8) & 0xFF);
    buffer.push_back(fcs & 0xFF);
    
    return buffer;
}

Frame Frame::deserialize(const std::vector<uint8_t>& buffer) {
    Frame f;
    if (buffer.size() < 21) return f; 
    
    memcpy(f.src_mac, buffer.data(), 6);
    memcpy(f.dest_mac, buffer.data() + 6, 6);
    
    f.length = (buffer[12] << 8) | buffer[13];
    f.seq_no = buffer[14];
    f.type = static_cast<FrameType>(buffer[15]);
    f.ack_seq_no = buffer[16];
    
    size_t payload_len = buffer.size() - 21;
    if (payload_len > 0) {
        f.payload.assign(buffer.begin() + 17, buffer.end() - 4);
    }
    
    f.fcs = (static_cast<uint32_t>(buffer[buffer.size()-4]) << 24) | 
            (static_cast<uint32_t>(buffer[buffer.size()-3]) << 16) | 
            (static_cast<uint32_t>(buffer[buffer.size()-2]) << 8) | 
             static_cast<uint32_t>(buffer[buffer.size()-1]);
    return f;
}

void Frame::calculate_fcs(int scheme) {
    std::vector<uint8_t> temp = serialize();
    temp.resize(temp.size() - 4); // exclude the 4 trailing bytes
    if (scheme == 1) {
        fcs = calculate_checksum(temp);
    } else {
        fcs = calculate_crc32(temp);
    }
}

bool Frame::verify_fcs(int scheme) const {
    std::vector<uint8_t> temp = serialize();
    temp.resize(temp.size() - 4);
    uint32_t calculated;
    if (scheme == 1) {
        calculated = calculate_checksum(temp);
    } else {
        calculated = calculate_crc32(temp);
    }
    return fcs == calculated;
}
