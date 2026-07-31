/* sender.c
 * ------------------------------------------------------------
 * Sender program - now sends the frame over a real TCP network
 * connection instead of writing to a local file, so this can run
 * between two different Linux machines.
 *
 * Usage:
 *   ./sender <input_file> <crc_width: 8|10|16|32> <error_type: 0-4> <receiver_ip> <receiver_port>
 *
 * error_type:
 *   0 = no error (clean transmission)
 *   1 = single-bit error
 *   2 = two isolated bit errors
 *   3 = odd number of bit errors
 *   4 = burst error
 *
 * Steps:
 *   1. Read the input file as the dataword (payload). Pad it with
 *      zero bytes up to the Ethernet minimum (46 bytes) if it is
 *      short, or use only the first 1500 bytes (Ethernet maximum)
 *      if it is long.
 *   2. Compute the 16-bit checksum and the chosen CRC over the
 *      payload.
 *   3. Assemble the frame = payload + checksum + CRC.
 *   4. Inject the requested error into the frame.
 *   5. Open a TCP connection to the receiver (IP + port given on
 *      the command line) and send the frame over the network.
 * ------------------------------------------------------------
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "defs.h"
#include "checksum.h"
#include "crc.h"
#include "error.h"
#include "hex.h"

int main(int argc, char *argv[]) {
    if (argc != 6) {
        printf("Usage: %s <input_file> <crc_width:8|10|16|32> <error_type:0-4> <receiver_ip> <receiver_port>\n", argv[0]);
        printf("error_type: 0=none 1=single-bit 2=two-isolated 3=odd 4=burst\n");
        return 1;
    }

    char *input_file = argv[1];
    int crc_width = atoi(argv[2]);
    int error_type = atoi(argv[3]);
    char *receiver_ip = argv[4];
    int receiver_port = atoi(argv[5]);

    if (crc_width != 8 && crc_width != 10 && crc_width != 16 && crc_width != 32) {
        printf("Error: crc_width must be 8, 10, 16 or 32\n");
        return 1;
    }
    if (error_type < 0 || error_type > 4) {
        printf("Error: error_type must be 0, 1, 2, 3 or 4\n");
        return 1;
    }

    /* ---------- Step 1: read the input file as the payload ---------- */
    FILE *fin = fopen(input_file, "rb");
    if (fin == NULL) {
        printf("Error: could not open input file '%s'\n", input_file);
        return 1;
    }

    unsigned char payload[MAX_PAYLOAD];
    memset(payload, 0, sizeof(payload));
    int file_bytes = (int)fread(payload, 1, MAX_PAYLOAD, fin);
    fclose(fin);

    int payload_len = file_bytes;
    if (payload_len < MIN_PAYLOAD)
        payload_len = MIN_PAYLOAD;      /* rest is already zero-padded */
    if (payload_len % 2 != 0)
        payload_len++;                  /* keep it even for the 16-bit checksum */

    printf("Read %d bytes from '%s' -> payload length used = %d bytes\n",
           file_bytes, input_file, payload_len);

    /* ---------- Step 2: compute checksum and CRC over the payload ---------- */
    uint16_t checksum = compute_checksum(payload, payload_len);

    crc_poly_t poly = get_crc_poly(crc_width);
    uint32_t crc_value = compute_crc(payload, payload_len, poly);
    int crc_bytes = (crc_width + 7) / 8;

    printf("Computed checksum = 0x%04x\n", checksum);
    printf("Computed CRC-%d    = 0x%0*x\n", crc_width, crc_bytes * 2, crc_value);

    /* ---------- Step 3: assemble the frame = payload + checksum + crc ---------- */
    unsigned char frame[MAX_PAYLOAD + 2 + 4];
    memcpy(frame, payload, payload_len);
    frame[payload_len]     = (unsigned char)(checksum >> 8);
    frame[payload_len + 1] = (unsigned char)(checksum & 0xFF);
    int i;
    for (i = 0; i < crc_bytes; i++) {
        int shift = 8 * (crc_bytes - 1 - i);
        frame[payload_len + 2 + i] = (unsigned char)((crc_value >> shift) & 0xFF);
    }
    int frame_len = payload_len + 2 + crc_bytes;

    /* ---------- Step 4: inject the requested error ---------- */
    srand((unsigned int)time(NULL) ^ (unsigned int)getpid());
    int total_bits = frame_len * 8;

    switch (error_type) {
        case 0:
            printf("No error injected (clean transmission)\n");
            break;
        case 1:
            single_bit_error(frame, total_bits);
            printf("Injected: single-bit error\n");
            break;
        case 2:
            two_isolated_bit_errors(frame, total_bits);
            printf("Injected: two isolated bit errors\n");
            break;
        case 3:
            odd_bit_errors(frame, total_bits);
            printf("Injected: odd number of bit errors\n");
            break;
        case 4:
            burst_error(frame, total_bits, 8);   /* 8-bit burst */
            printf("Injected: burst error (length 8 bits)\n");
            break;
    }

    /* ---------- Step 5: send the frame to the receiver over TCP ---------- */
    char payload_hex[2 * MAX_PAYLOAD + 1];
    char checksum_hex[5];
    char crc_hex[9];
    bytes_to_hex(frame, payload_len, payload_hex);
    bytes_to_hex(frame + payload_len, 2, checksum_hex);
    bytes_to_hex(frame + payload_len + 2, crc_bytes, crc_hex);

    /* 1. Create a TCP socket (IPv4, stream = TCP) */
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        return 1;
    }

    /* 2. Fill in the receiver's address (IP + port) */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((uint16_t)receiver_port);   /* host to network byte order */
    if (inet_pton(AF_INET, receiver_ip, &server_addr.sin_addr) <= 0) {
        printf("Error: invalid receiver IP address '%s'\n", receiver_ip);
        close(sock_fd);
        return 1;
    }

    /* 3. Connect to the receiver */
    printf("Connecting to %s:%d ...\n", receiver_ip, receiver_port);
    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock_fd);
        return 1;
    }

    /* 4. Wrap the socket as a FILE* so we can just use fprintf,
          exactly like we did when writing to channel.txt */
    FILE *fout = fdopen(sock_fd, "w");
    if (fout == NULL) {
        perror("fdopen");
        close(sock_fd);
        return 1;
    }
    fprintf(fout, "PAYLOAD_LEN %d\n", payload_len);
    fprintf(fout, "CRC_WIDTH %d\n", crc_width);
    fprintf(fout, "PAYLOAD %s\n", payload_hex);
    fprintf(fout, "CHECKSUM %s\n", checksum_hex);
    fprintf(fout, "CRC %s\n", crc_hex);
    fflush(fout);           /* make sure everything is actually sent   */
    shutdown(sock_fd, SHUT_WR); /* tell the receiver "no more data coming" */
    fclose(fout);           /* this also closes sock_fd                */

    printf("Frame sent over the network to %s:%d\n", receiver_ip, receiver_port);
    return 0;
}
