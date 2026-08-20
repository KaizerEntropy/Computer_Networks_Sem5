#include "utils.hpp"
#include "checksum.hpp"
#include "crc.hpp"
#include <fstream>
#include <cstring>
#include <cctype>

std::vector<uint8_t> read_text_file(const std::string &path) {
    std::ifstream f(path, std::ios::binary);
    std::vector<uint8_t> data;
    if (!f.is_open()) {
        return data;
    }
    
    char c;
    while (f.get(c)) {
        data.push_back((uint8_t)c);
    }
    f.close();
    return data;
}

std::vector<uint8_t> read_bits_file(const std::string &path) {
    std::ifstream f(path);
    std::vector<uint8_t> result;
    if (!f.is_open()) {
        return result;
    }
    
    std::vector<int> bits;
    char ch;
    while (f.get(ch)) {
        if (ch == '0') {
            bits.push_back(0);
        } else if (ch == '1') {
            bits.push_back(1);
        }
    }
    f.close();
    
    if (bits.size() == 0) return result;
    
    size_t nbytes = (bits.size() + 7) / 8;
    for (size_t i = 0; i < nbytes; i++) {
        result.push_back(0);
    }
    
    for (size_t i = 0; i < bits.size(); i++) {
        if (bits[i] == 1) {
            result[i/8] = result[i/8] | (uint8_t)(0x80 >> (i % 8));
        }
    }
    return result;
}

std::string bytes_to_bits_string(const std::vector<uint8_t> &data) {
    std::string out = "";
    for (size_t i = 0; i < data.size(); i++) {
        for (int b = 7; b >= 0; b--) {
            if ((data[i] >> b) & 1) {
                out = out + "1";
            } else {
                out = out + "0";
            }
        }
        if ((i + 1) % 8 == 0) {
            out = out + "\n";
        } else if (i + 1 < data.size()) {
            out = out + " ";
        }
    }
    if (out.length() > 0 && out.back() != '\n') {
        out = out + "\n";
    }
    return out;
}

FileType detectFileType(const std::string &filename) {
    size_t dot = filename.rfind('.');
    if (dot != std::string::npos) {
        std::string ext = filename.substr(dot);
        for (size_t i=0; i<ext.length(); i++) {
            ext[i] = (char)tolower((unsigned char)ext[i]);
        }
        if (ext == ".bits") {
            return FILE_BITS;
        }
    }
    return FILE_TEXT;
}

std::vector<uint8_t> read_input_file(const std::string &path, FileType ft) {
    if (ft == FILE_BITS) {
        return read_bits_file(path);
    } else {
        return read_text_file(path);
    }
}

bool send_all(int sockfd, const void *data, size_t size) {
    const uint8_t *p = (const uint8_t *)data;
    size_t sent = 0;
    while (sent < size) {
        ssize_t n = send(sockfd, p + sent, size - sent, 0);
        if (n <= 0) {
            return false;
        }
        sent = sent + (size_t)n;
    }
    return true;
}

bool recv_all(int sockfd, void *data, size_t size) {
    uint8_t *p = (uint8_t *)data;
    size_t got = 0;
    while (got < size) {
        ssize_t n = recv(sockfd, p + got, size - got, 0);
        if (n <= 0) {
            return false;
        }
        got = got + (size_t)n;
    }
    return true;
}

uint32_t fcs_compute(const uint8_t *data, int len, int scheme) {
    if (scheme == CHECKSUM16) {
        return compute_checksum(data, len);
    }
    
    int width = getSchemeInfo(scheme).width_bits;
    CrcPoly p = getCrcPoly(width);
    return compute_crc(data, len, p);
}

int fcs_width_bytes(int scheme) {
    int width = getSchemeInfo(scheme).width_bits;
    return (width + 7) / 8;
}

void fcs_pack(uint32_t val, int scheme, uint8_t fcs_out[4]) {
    for(int i=0; i<4; i++) fcs_out[i] = 0;
    
    int wb = fcs_width_bytes(scheme);
    for (int i = 0; i < wb; i++) {
        int shift = 8 * (wb - 1 - i);
        fcs_out[4 - wb + i] = (uint8_t)((val >> shift) & 0xFF);
    }
}

uint32_t fcs_unpack(const uint8_t fcs_in[4], int scheme) {
    int wb = fcs_width_bytes(scheme);
    uint32_t val = 0;
    for (int i = 0; i < wb; i++) {
        val = (val << 8) | fcs_in[4 - wb + i];
    }
    return val;
}

bool fcs_unused_clean(const uint8_t fcs_in[4], int scheme) {
    int wb = fcs_width_bytes(scheme);
    for (int i = 0; i < 4 - wb; i++) {
        if (fcs_in[i] != 0) return false;
    }
    return true;
}

Frame build_frame(const uint8_t *dest_mac, const uint8_t *src_mac, const uint8_t *chunk, int chunk_len, int scheme) {
    Frame f;
    
    for(int i=0; i<DEST_LEN; i++) f.dest[i] = dest_mac[i];
    for(int i=0; i<SRC_LEN; i++) f.src[i] = src_mac[i];
    
    f.length = (uint16_t)chunk_len;
    
    for(int i=0; i<PAYLOAD_SIZE; i++) f.payload[i] = 0; // Pad with zeros
    
    for(int i=0; i<chunk_len; i++) f.payload[i] = chunk[i];

    uint8_t coverage[FCS_COVER_LEN];
    int off = 0;
    
    for(int i=0; i<DEST_LEN; i++) coverage[off + i] = f.dest[i];
    off += DEST_LEN;
    
    for(int i=0; i<SRC_LEN; i++) coverage[off + i] = f.src[i];
    off += SRC_LEN;
    
    coverage[off] = (uint8_t)((f.length >> 8) & 0xFF);
    coverage[off + 1] = (uint8_t)(f.length & 0xFF);
    off += 2;
    
    for(int i=0; i<PAYLOAD_SIZE; i++) coverage[off + i] = f.payload[i];
    
    uint32_t val = fcs_compute(coverage, FCS_COVER_LEN, scheme);
    fcs_pack(val, scheme, f.fcs);
    return f;
}
