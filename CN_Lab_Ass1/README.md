# Assignment 1 — Error Detection (Checksum & CRC) via Socket Programming

A C++17 implementation of error detection using **16-bit Checksum** and
**CRC-8/10/16/32** over TCP sockets, with an IEEE 802.3-style 128-byte frame.

## Frame Layout (128 bytes)

```
Offset   Size    Field
------   ----    -----
0        6       Destination MAC Address
6        6       Source MAC Address
12       2       Length (valid payload bytes, ≤ 110)
14       110     Payload / Data (zero-padded if short)
124      4       FCS (Frame Check Sequence — always 4 bytes)
------
128      TOTAL
```

## Files

```
common.hpp              Shared header (frame layout, checksum, CRC, error injector)
sender.cpp              Sender program
receiver.cpp            Receiver program
Makefile                Build script
message.txt             Default text input (2916 bytes, spans 27 frames)
sample_input.txt        Short sample (913 bytes, 9 frames)
checksum_blind_test.bits   .bits file for checksum blind-spot demo
crc_blind_test.bits        .bits file for CRC blind-spot demo
run_all_tests.sh        Automated test harness (all 5 schemes)
```

## Build

```bash
make
```

## Run (two separate terminals)

### Terminal 1 — Receiver
```bash
./receiver 9000
```

### Terminal 2 — Sender
```bash
# Text file:
./sender 127.0.0.1 9000 message.txt

# .bits file:
./sender 127.0.0.1 9000 checksum_blind_test.bits

# On a different computer (replace with receiver's actual IP):
./sender 192.168.1.10 9000 message.txt
```

## Detection Schemes

| Code | Scheme | Polynomial |
|------|--------|-----------|
| 1 | 16-bit Checksum | One's complement sum |
| 2 | CRC-8 | x⁸+x⁷+x⁶+x⁴+x²+1 |
| 3 | CRC-10 | x¹⁰+x⁹+x⁵+x⁴+x+1 |
| 4 | CRC-16 | x¹⁶+x¹⁵+x²+1 |
| 5 | CRC-32 | IEEE 802.3 Ethernet |

## Round-Robin Error Injection

Every transmission cycles through all 7 error types:

| Frame (mod 7) | Error Type |
|---|---|
| 1 | No error (clean) |
| 2 | Single-bit error |
| 3 | Two isolated bit errors |
| 4 | Odd number of bit errors |
| 5 | Burst error (8-bit) |
| 6 | Checksum-blind (CRC catches, checksum misses) |
| 7 | CRC-blind (checksum catches, CRC misses) |

## Key Results

- **Frame 6 with Checksum**: Checksum **MISSES** → ACK (error undetected)
- **Frame 6 with CRC-32**: CRC **CATCHES** → NACK
- **Frame 7 with CRC-32**: CRC **MISSES** → ACK (error undetected)
- **Frame 7 with Checksum**: Checksum **CATCHES** → NACK

This directly demonstrates the assignment requirement:
1. Errors detected by **both** CRC and Checksum (frames 2, 3, 4, 5)
2. Errors detected by **CRC but not Checksum** (frame 6)
3. Errors detected by **Checksum but not CRC** (frame 7)
