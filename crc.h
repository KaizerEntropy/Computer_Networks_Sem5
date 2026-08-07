/* crc.h - simple CRC module
   One function computes the CRC for any of the 4 required
   polynomials, just by passing a different (poly, width) pair. */
#ifndef CRC_H
#define CRC_H

#include <stdint.h>

typedef struct {
    uint32_t poly;   /* generator polynomial, without its leading term */
    int width;       /* degree of the polynomial: 8, 10, 16 or 32      */
} crc_poly_t;

/* Returns the standard polynomial for the requested width (8/10/16/32). */
crc_poly_t get_crc_poly(int width);

/* Computes the CRC remainder of data[0..len) using generator p.
   Returns the remainder right-justified (e.g. an 8-bit CRC is
   returned in the low 8 bits of the uint32_t). */
uint32_t compute_crc(unsigned char data[], int len, crc_poly_t p);

#endif
