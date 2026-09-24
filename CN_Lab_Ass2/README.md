# Assignment 2 - Flow Control Mechanisms

This project implements three Flow Control mechanisms at the Data Link Layer as specified in Assignment 2:
1. Stop and Wait ARQ
2. Go-Back-N ARQ
3. Selective Repeat ARQ

The implementation uses UDP Sockets (`SOCK_DGRAM`) rather than TCP to properly simulate network frames and boundaries without the underlying TCP automatic retransmissions and stream behavior.

## Implementation Details
- **Languages/Tools**: C++11, POSIX Sockets, pthreads (via `<thread>`).
- **Frame Structure**: Header (Src MAC, Dest MAC, Length, Seq No, Frame Type, ACK Seq No), Payload (variable size up to 1500 bytes), Trailer (FCS/CRC).
- **Separate Devices Communication**: The server (Receiver) binds to `INADDR_ANY` (all available interfaces), and the client (Sender) connects to the user-specified Receiver IP Address. By entering the IP Address of a second computer on the same network (e.g., `192.168.1.100`), two separate devices can communicate with each other over this socket program.
- **Random Delays and Packet Loss**: Simulated probabilistically based on user input. Random values drop packets on the sender side and ACK packets on the receiver side to test the timeouts and retransmission behavior.

## How to Compile
```bash
make
```

## How to Run
We need two separate terminals (or two separate computers).

**1. Run the Receiver First:**
```bash
./receiver <PORT>
```
*Example:*
```bash
./receiver 9000
```
It will ask you to select the Protocol (1: Stop and Wait, 2: Go-Back-N, 3: Selective Repeat), the Window Size, and the Probability of ACK Loss.

**2. Run the Sender:**
```bash
./sender <RECEIVER_IP> <PORT> <INPUT_FILE>
```
*Example (Running on the same computer):*
```bash
./sender 127.0.0.1 9000 input.txt
```
*Example (Running on two separate computers):*
If the receiver's IP address on the network is `192.168.1.55`:
```bash
./sender 192.168.1.55 9000 input.txt
```
It will ask you for the Protocol, Window Size, Probability of Packet Loss, and Probability of Bit Error.

## Output
- The `sender` will log frame transmissions, timeouts, and ACKs received.
- The `receiver` will log frames received, buffered out-of-order frames, and ACKs sent.
- After receiving the complete file, the receiver will save the reconstructed file as `received_file.txt`.
