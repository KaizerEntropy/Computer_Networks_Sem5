/* receiver.c
 * ------------------------------------------------------------
 * Receiver program - now a TCP server. It listens on a port,
 * and every time a sender connects and sends a frame, it checks
 * that frame and prints the result. It then goes back to
 * listening, so you can run the sender multiple times against
 * the same receiver without restarting it (Ctrl+C to stop).
 *
 * Usage:
 *   ./receiver <port>
 *
 * Steps:
 *   1. Create a TCP socket, bind it to the given port, and listen.
 *   2. Accept a connection from the sender and read the frame
 *      (payload, the 16-bit checksum field, and the CRC field,
 *      exactly as they arrived -- possibly corrupted by the
 *      sender's error injection step).
 *   3. Checksum check: recompute the checksum over the received
 *      payload + received checksum field; accept if the result is
 *      all-ones.
 *   4. CRC check: recompute the CRC over the received payload and
 *      compare it against the received CRC field; accept if they
 *      match.
 *   5. Print ACCEPTED/REJECTED for each scheme, then go back to
 *      step 2 and wait for the next sender.
 * ------------------------------------------------------------
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "defs.h"
#include "checksum.h"
#include "crc.h"
#include "hex.h"

/* Reads one frame from fin (a FILE* wrapping the connected socket)
   and checks it with both schemes. Same logic as the file-based
   version -- only where the data comes from has changed. */
void process_frame(FILE *fin) {
    char label[32];
    int payload_len, crc_width;
    char payload_hex[2 * MAX_PAYLOAD + 1];
    char checksum_hex[16];
    char crc_hex[16];

    if (fscanf(fin, "%s %d", label, &payload_len) != 2) return;
    if (fscanf(fin, "%s %d", label, &crc_width) != 2) return;
    if (fscanf(fin, "%s %s", label, payload_hex) != 2) return;
    if (fscanf(fin, "%s %s", label, checksum_hex) != 2) return;
    if (fscanf(fin, "%s %s", label, crc_hex) != 2) return;

    printf("Received frame: payload_len=%d bytes, crc_width=%d bits\n", payload_len, crc_width);

    /* ---------- rebuild the received bytes ---------- */
    unsigned char payload[MAX_PAYLOAD];
    unsigned char received_checksum[2];
    int crc_bytes = (crc_width + 7) / 8;
    unsigned char received_crc_field[4];

    hex_to_bytes(payload_hex, payload, payload_len);
    hex_to_bytes(checksum_hex, received_checksum, 2);
    hex_to_bytes(crc_hex, received_crc_field, crc_bytes);

    /* ---------- checksum check ---------- */
    unsigned char cs_buf[MAX_PAYLOAD + 2];
    memcpy(cs_buf, payload, payload_len);
    memcpy(cs_buf + payload_len, received_checksum, 2);
    int checksum_ok = check_checksum(cs_buf, payload_len + 2);

    /* ---------- CRC check ---------- */
    crc_poly_t poly = get_crc_poly(crc_width);
    uint32_t recomputed_crc = compute_crc(payload, payload_len, poly);

    uint32_t received_crc = 0;
    int i;
    for (i = 0; i < crc_bytes; i++)
        received_crc = (received_crc << 8) | received_crc_field[i];

    int crc_ok = (recomputed_crc == received_crc);

    /* ---------- report ---------- */
    printf("\n--- Checksum scheme ---\n");
    printf("Received checksum field : 0x%02x%02x\n", received_checksum[0], received_checksum[1]);
    printf("Result                  : %s\n", checksum_ok ? "ACCEPTED (no error detected)" : "REJECTED (error detected)");

    printf("\n--- CRC-%d scheme ---\n", crc_width);
    printf("Received CRC field      : 0x%0*x\n", crc_bytes * 2, received_crc);
    printf("Recomputed CRC          : 0x%0*x\n", crc_bytes * 2, recomputed_crc);
    printf("Result                  : %s\n", crc_ok ? "ACCEPTED (no error detected)" : "REJECTED (error detected)");

    printf("\n--- Overall ---\n");
    if (checksum_ok && crc_ok)
        printf("Both schemes ACCEPT the frame.\n");
    else if (!checksum_ok && !crc_ok)
        printf("Both schemes REJECT the frame (error detected by both).\n");
    else if (!checksum_ok && crc_ok)
        printf("Only the CHECKSUM detected an error; CRC did not.\n");
    else
        printf("Only the CRC detected an error; checksum did not.\n");
    printf("\n");
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IOLBF, 0);  /* flush stdout after every line, even when redirected to a file */

    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }
    int port = atoi(argv[1]);

    /* 1. Create a TCP socket */
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }

    /* Allow the port to be reused immediately after this program
       exits (otherwise Linux keeps it reserved for a minute or so) */
    int yes = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    /* 2. Bind the socket to the given port on all local interfaces */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;   /* accept on any local IP */
    server_addr.sin_port = htons((uint16_t)port);

    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(listen_fd);
        return 1;
    }

    /* 3. Start listening for incoming connections (queue up to 5) */
    if (listen(listen_fd, 5) < 0) {
        perror("listen");
        close(listen_fd);
        return 1;
    }

    printf("Receiver listening on port %d ... (Ctrl+C to stop)\n\n", port);

    /* 4. Loop forever: accept one sender, process its frame, repeat */
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (conn_fd < 0) {
            perror("accept");
            continue;
        }

        printf("Connection accepted from %s\n", inet_ntoa(client_addr.sin_addr));

        FILE *fin = fdopen(conn_fd, "r");
        if (fin != NULL) {
            process_frame(fin);
            fclose(fin);   /* this also closes conn_fd */
        } else {
            close(conn_fd);
        }
        fflush(stdout);
    }

    close(listen_fd);
    return 0;
}

