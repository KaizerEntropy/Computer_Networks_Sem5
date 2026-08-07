/* crc.c - textbook bit-by-bit CRC (polynomial long division over GF(2)) */
#include "crc.h"

crc_poly_t get_crc_poly(int width) {
    crc_poly_t p;
    p.width = width;
    switch (width) {
        case 8:  p.poly = 0xD5;       break; /* x^8 +x^7+x^6+x^4+x^2+1     */
        case 10: p.poly = 0x233;      break; /* x^10+x^9+x^5+x^4+x+1       */
        case 16: p.poly = 0x8005;     break; /* x^16+x^15+x^2+1  (USB)     */
        case 32: p.poly = 0x04C11DB7; break; /* IEEE 802.3 Ethernet        */
        default: p.poly = 0;          break;
    }
    return p;
}

uint32_t compute_crc(unsigned char data[], int len, crc_poly_t p) {
    uint32_t mask = (p.width == 32) ? 0xFFFFFFFFu : ((1u << p.width) - 1u);
    uint32_t topbit = 1u << (p.width - 1);
    uint32_t reg = 0;               /* shift register, starts at 0 */
    int i;

    /* feed the message in, one bit at a time, MSB of each byte first */
    for (i = 0; i < len * 8; i++) {
        int byte_index = i / 8;
        int bit_index = 7 - (i % 8);
        uint32_t bit = (data[byte_index] >> bit_index) & 1;

        uint32_t carry = (reg & topbit) ? 1 : 0;
        reg = ((reg << 1) | bit) & mask;
        if (carry)
            reg = reg ^ p.poly;
    }
    return reg;
}
