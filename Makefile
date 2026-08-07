CC = gcc
CFLAGS = -Wall -std=c11

all: sender receiver

sender: sender.c checksum.c crc.c error.c hex.c
	$(CC) $(CFLAGS) -o sender sender.c checksum.c crc.c error.c hex.c

receiver: receiver.c checksum.c crc.c hex.c
	$(CC) $(CFLAGS) -o receiver receiver.c checksum.c crc.c hex.c

clean:
	rm -f sender receiver channel.txt
