/* error.h - functions to deliberately corrupt a byte array
   (simulating errors introduced by a noisy channel) */
#ifndef ERROR_H
#define ERROR_H

#include <stddef.h>

/* flips a single bit; bit_pos counted from the MSB of byte 0 */
void flip_bit(unsigned char data[], int bit_pos);

/* introduces exactly one bit error at a random position */
void single_bit_error(unsigned char data[], int total_bits);

/* introduces two bit errors that are NOT next to each other */
void two_isolated_bit_errors(unsigned char data[], int total_bits);

/* introduces an odd number of bit errors (3, 5 or 7, chosen at random) */
void odd_bit_errors(unsigned char data[], int total_bits);

/* introduces a burst error: a run of `burst_len` bits where the
   first and last bit of the run are always flipped and the bits in
   between are flipped at random */
void burst_error(unsigned char data[], int total_bits, int burst_len);

#endif
