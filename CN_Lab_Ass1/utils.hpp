#ifndef UTILS_HPP
#define UTILS_HPP
#include <vector>
#include <string>
#include <cstdint>
#include <sys/socket.h>
#include "frame.hpp"

FileType detectFileType(const std::string &filename);
std::vector<uint8_t> read_input_file(const std::string &path, FileType ft);
std::string bytes_to_bits_string(const std::vector<uint8_t> &data);
bool send_all(int sockfd, const void *data, size_t size);
bool recv_all(int sockfd, void *data, size_t size);

uint32_t fcs_compute(const uint8_t *data, int len, int scheme);
int fcs_width_bytes(int scheme);
void fcs_pack(uint32_t val, int scheme, uint8_t fcs_out[4]);
uint32_t fcs_unpack(const uint8_t fcs_in[4], int scheme);
bool fcs_unused_clean(const uint8_t fcs_in[4], int scheme);
Frame build_frame(const uint8_t *dest_mac, const uint8_t *src_mac, const uint8_t *chunk, int chunk_len, int scheme);
#endif
