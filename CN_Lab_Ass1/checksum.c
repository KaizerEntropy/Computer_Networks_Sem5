/* checksum.c - simple 16-bit one's complement checksum */
#include "checksum.h"

uint16_t compute_checksum(unsigned char data[], int len) {
    uint32_t sum = 0;
    int i;

    /* add data two bytes (16 bits) at a time */
    for (i = 0; i + 1 < len; i = i + 2) {
        uint16_t word = (data[i] << 8) | data[i + 1];
        sum = sum + word;
        if (sum > 0xFFFF)              /* end-around carry */
            sum = (sum & 0xFFFF) + 1;
    }
    if (i < len) {                      /* one leftover byte */
        uint16_t word = data[i] << 8;
        sum = sum + word;
        if (sum > 0xFFFF)
            sum = (sum & 0xFFFF) + 1;
    }

    return (uint16_t)(~sum & 0xFFFF);   /* checksum = complement of sum */
}

int check_checksum(unsigned char data[], int len) {
    uint32_t sum = 0;
    int i;

    for (i = 0; i + 1 < len; i = i + 2) {
        uint16_t word = (data[i] << 8) | data[i + 1];
        sum = sum + word;
        if (sum > 0xFFFF)
            sum = (sum & 0xFFFF) + 1;
    }
    if (i < len) {
        uint16_t word = data[i] << 8;
        sum = sum + word;
        if (sum > 0xFFFF)
            sum = (sum & 0xFFFF) + 1;
    }

    /* if nothing changed on the way, sum of (data + checksum) is all 1s */
    return (sum & 0xFFFF) == 0xFFFF;
}
