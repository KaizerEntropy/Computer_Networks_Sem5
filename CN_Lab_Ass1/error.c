/* error.c */
#include "error.h"
#include <stdlib.h>

void flip_bit(unsigned char data[], int bit_pos) {
    int byte_index = bit_pos / 8;
    int bit_index = bit_pos % 8;
    data[byte_index] = data[byte_index] ^ (0x80 >> bit_index);
}

void single_bit_error(unsigned char data[], int total_bits) {
    int pos = rand() % total_bits;
    flip_bit(data, pos);
}

void two_isolated_bit_errors(unsigned char data[], int total_bits) {
    int pos1 = rand() % total_bits;
    int pos2;
    do {
        pos2 = rand() % total_bits;
    } while (abs(pos2 - pos1) < 2);   /* keep them apart, i.e. isolated */

    flip_bit(data, pos1);
    flip_bit(data, pos2);
}

void odd_bit_errors(unsigned char data[], int total_bits) {
    int choices[3] = {3, 5, 7};
    int count = choices[rand() % 3];
    int flipped[7];   /* max possible count above is 7 */
    int n = 0;

    while (n < count) {
        int pos = rand() % total_bits;
        int already_used = 0;
        int j;
        for (j = 0; j < n; j++) {
            if (flipped[j] == pos) {
                already_used = 1;
                break;
            }
        }
        if (!already_used) {
            flipped[n] = pos;
            n++;
        }
    }
    int k;
    for (k = 0; k < count; k++)
        flip_bit(data, flipped[k]);
}

void burst_error(unsigned char data[], int total_bits, int burst_len) {
    int start = rand() % (total_bits - burst_len + 1);
    int i;

    flip_bit(data, start);                    /* first bit of the burst  */
    flip_bit(data, start + burst_len - 1);     /* last bit of the burst   */
    for (i = 1; i < burst_len - 1; i++) {      /* bits in between: random */
        if (rand() % 2 == 1)
            flip_bit(data, start + i);
    }
}
