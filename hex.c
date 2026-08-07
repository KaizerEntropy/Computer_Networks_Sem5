/* hex.c */
#include "hex.h"
#include <stdio.h>

void bytes_to_hex(unsigned char data[], int len, char hexout[]) {
    int i;
    for (i = 0; i < len; i++)
        sprintf(hexout + i * 2, "%02x", data[i]);
    hexout[len * 2] = '\0';
}

void hex_to_bytes(const char hexstr[], unsigned char data[], int len) {
    int i;
    for (i = 0; i < len; i++) {
        unsigned int byte;
        sscanf(hexstr + i * 2, "%2x", &byte);
        data[i] = (unsigned char)byte;
    }
}
