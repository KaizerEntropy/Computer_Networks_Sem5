#include "error.hpp"
#include "frame.hpp"
#include "crc.hpp"
#include <sstream>
#include <cstdlib> // For rand() and srand()

std::string errorTypeName(int e) {
    if (e == 0) return "No error (clean)";
    if (e == 1) return "Single-bit error";
    if (e == 2) return "Two isolated bit errors";
    if (e == 3) return "Odd number of bit errors";
    if (e == 4) return "Burst error (8-bit)";
    if (e == 5) return "Checksum-blind error";
    if (e == 6) return "CRC-blind error";
    return "UNKNOWN";
}

int rand_range(int lo, int hi) {
    int range = hi - lo;
    if (range <= 0) return lo;
    return lo + (rand() % range);
}

void flip_bit(uint8_t *data, int bit_pos) {
    int byte_pos = bit_pos / 8;
    int bit_offset = bit_pos % 8;
    uint8_t mask = (0x80 >> bit_offset);
    data[byte_pos] = data[byte_pos] ^ mask;
}

std::string single_bit(uint8_t *data, int total_bits) {
    int pos = rand_range(0, total_bits);
    flip_bit(data, pos);
    
    std::ostringstream oss;
    oss << "Flipped bit " << pos << " (byte " << pos/8 << ", bit " << pos%8 << ")";
    return oss.str();
}

std::string two_isolated(uint8_t *data, int total_bits) {
    int p1 = rand_range(0, total_bits);
    int p2 = p1;
    
    while (abs(p2 - p1) < 2) {
        p2 = rand_range(0, total_bits);
    }
    
    flip_bit(data, p1);
    flip_bit(data, p2);
    
    std::ostringstream oss;
    oss << "Flipped bits " << p1 << " and " << p2;
    return oss.str();
}

std::string odd_bits(uint8_t *data, int total_bits) {
    int choices[3] = {3, 5, 7};
    int count = choices[rand_range(0, 3)];
    
    int positions[7];
    int filled = 0;
    
    while (filled < count) {
        int p = rand_range(0, total_bits);
        bool duplicate = false;
        for (int i = 0; i < filled; i++) {
            if (positions[i] == p) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) {
            positions[filled] = p;
            filled++;
        }
    }
    
    for (int i = 0; i < filled; i++) {
        flip_bit(data, positions[i]);
    }
    
    std::ostringstream oss;
    oss << "Flipped " << count << " bits";
    return oss.str();
}

std::string burst(uint8_t *data, int total_bits, int burst_len) {
    int start = rand_range(0, total_bits - burst_len + 1);
    
    // Always flip first and last to guarantee burst length
    flip_bit(data, start);
    flip_bit(data, start + burst_len - 1);
    
    // Randomize the bits in between
    for (int i = 1; i < burst_len - 1; i++) {
        if (rand_range(0, 2) == 1) {
            flip_bit(data, start + i);
        }
    }
    
    std::ostringstream oss;
    oss << burst_len << "-bit burst starting at bit " << start;
    return oss.str();
}

std::string checksum_blind(uint8_t *frame) {
    int pstart = HEADER_LEN;
    int pend = FCS_COVER_LEN;
    
    // Simple way to blind checksum: increment one word, decrement the next
    for (int off = pstart; off + 3 < pend; off += 2) {
        uint16_t w1 = (uint16_t)((frame[off]<<8) | frame[off+1]);
        uint16_t w2 = (uint16_t)((frame[off+2]<<8) | frame[off+3]);
        
        if (w1 < 0xFFFF && w2 > 0) {
            w1 = w1 + 1;
            w2 = w2 - 1;
            frame[off] = (uint8_t)(w1 >> 8); 
            frame[off+1] = (uint8_t)(w1 & 0xFF);
            frame[off+2] = (uint8_t)(w2 >> 8); 
            frame[off+3] = (uint8_t)(w2 & 0xFF);
            return "Checksum-blind: balanced word increment/decrement";
        }
    }
    return "Checksum-blind: no suitable pattern found";
}

std::string crc_blind(uint8_t *frame, int scheme) {
    int width = getSchemeInfo(scheme).width_bits;
    if (scheme == CHECKSUM16) width = 32;
    
    CrcPoly p = getCrcPoly(width);
    int start_bit = FCS_COVER_LEN * 8 - (width + 1);
    int flips = 1;
    
    flip_bit(frame, start_bit);
    
    for (int b = 0; b < width; b++) {
        if ((p.poly & (1u << (width - 1 - b))) != 0) {
            flip_bit(frame, start_bit + 1 + b);
            flips++;
        }
    }
    
    std::ostringstream oss;
    oss << "CRC-blind: generator-poly pattern (" << flips << " bits flipped)";
    return oss.str();
}

InjectionResult inject_error(uint8_t *frame, int frame_bytes, int error_type, int scheme) {
    int total_bits = frame_bytes * 8;
    
    if (error_type == ERR_NONE) return {false, "No error"};
    if (error_type == ERR_SINGLE_BIT) return {true, single_bit(frame, total_bits)};
    if (error_type == ERR_TWO_ISOLATED) return {true, two_isolated(frame, total_bits)};
    if (error_type == ERR_ODD_BITS) return {true, odd_bits(frame, total_bits)};
    if (error_type == ERR_BURST) return {true, burst(frame, total_bits, 8)};
    if (error_type == ERR_CHECKSUM_MISS) return {true, checksum_blind(frame)};
    if (error_type == ERR_CRC_MISS) return {true, crc_blind(frame, scheme)};
    
    return {false, "No error"};
}
