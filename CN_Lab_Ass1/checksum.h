/* checksum.h - simple 16-bit checksum module */
#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <stdint.h>
#include <stddef.h>

/* Adds all 16-bit words of data[0..len) with end-around carry
   (one's complement addition), and returns the checksum, which is
   the complement of that sum. len does not include the checksum. */
uint16_t compute_checksum(unsigned char data[], int len);

/* Receiver side check: pass the full received data INCLUDING the
   2 checksum bytes at the end. Returns 1 if OK (no error found),
   0 if the checksum does not match (error detected). */
int check_checksum(unsigned char data[], int len);

#endif
