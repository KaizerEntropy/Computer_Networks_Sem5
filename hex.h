/* hex.h - tiny helper to convert bytes to/from a hex string,
   used so the channel file can be a plain, human-readable text file */
#ifndef HEX_H
#define HEX_H

/* Writes 2*len hex characters (plus a null terminator) into hexout. */
void bytes_to_hex(unsigned char data[], int len, char hexout[]);

/* Reads 2*len hex characters from hexstr into data[0..len). */
void hex_to_bytes(const char hexstr[], unsigned char data[], int len);

#endif
