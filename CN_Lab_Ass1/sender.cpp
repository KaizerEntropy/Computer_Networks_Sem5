#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <cstring>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include "frame.hpp"
#include "error.hpp"
#include "utils.hpp"

std::vector<int> build_error_plan(int total_frames) {
    int cycle[ROUND_ROBIN_CYCLE] = {
        ERR_NONE, ERR_SINGLE_BIT, ERR_TWO_ISOLATED,
        ERR_ODD_BITS, ERR_BURST, ERR_CHECKSUM_MISS, ERR_CRC_MISS
    };
    std::vector<int> plan;
    for (int i = 0; i < total_frames; i++) {
        plan.push_back(cycle[i % ROUND_ROBIN_CYCLE]);
    }
    return plan;
}

int main(int argc, char *argv[]) {
    // Initialize simple random seed for our novice error injector
    srand((unsigned)time(0));
    
    std::cout << "\n--------------------------------------------------------\n";
    std::cout <<   "      SENDER - Error Detection Socket Programming       \n";
    std::cout <<   "--------------------------------------------------------\n\n";

    std::string receiver_ip;
    int port;
    std::string input_file;

    if (argc >= 4) {
        receiver_ip = argv[1];
        port = std::stoi(argv[2]);
        input_file = argv[3];
    } else {
        std::cout << "Enter Receiver IP: ";
        std::cin >> receiver_ip;
        std::cout << "Enter Port: ";
        std::cin >> port;
        std::cout << "Enter Input File (.txt or .bits): ";
        std::cin >> input_file;
    }

    FileType ft = detectFileType(input_file);
    std::vector<uint8_t> message = read_input_file(input_file, ft);
    
    if (message.size() == 0) {
        std::cerr << "Error: Could not read file or file is empty.\n";
        return 1;
    }

    std::cout << "\nSelect error-detection scheme:\n";
    std::cout << "  1) 16-bit Checksum\n";
    std::cout << "  2) CRC-8\n";
    std::cout << "  3) CRC-10\n";
    std::cout << "  4) CRC-16\n";
    std::cout << "  5) CRC-32 (IEEE 802.3)\n";
    
    int scheme = 0;
    while (scheme < 1 || scheme > 5) {
        std::cout << "Enter choice (1-5): ";
        std::cin >> scheme;
    }

    uint8_t dest_mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    uint8_t src_mac[6]  = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

    int total_frames = message.size() / PAYLOAD_SIZE;
    if (message.size() % PAYLOAD_SIZE != 0) {
        total_frames = total_frames + 1;
    }
    
    std::vector<int> error_plan = build_error_plan(total_frames);

    int planned_errors = 0;
    for (size_t i = 0; i < error_plan.size(); i++) {
        if (error_plan[i] != ERR_NONE) {
            planned_errors++;
        }
    }

    std::cout << "--- TRANSMISSION INFO ---\n";
    std::cout << "Input file       : " << input_file << "\n";
    std::cout << "File type        : " << fileTypeName(ft) << "\n";
    std::cout << "Message size     : " << message.size() << " bytes\n";
    std::cout << "Payload/frame    : " << PAYLOAD_SIZE << " bytes\n";
    std::cout << "Total frames     : " << total_frames << "\n";
    std::cout << "Scheme           : " << getSchemeInfo(scheme).name << "\n";
    std::cout << "Error mode       : Round-robin (7-type cycle)\n";
    std::cout << "Frames w/ errors : " << planned_errors << " of " << total_frames << "\n";
    std::cout << "-------------------------\n\n";

    std::cout << "Connecting to " << receiver_ip << ":" << port << " ...\n";
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { 
        std::cout << "Socket creation failed.\n"; 
        return 1; 
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, receiver_ip.c_str(), &server_addr.sin_addr);

    if (connect(sockfd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cout << "Connection failed.\n";
        return 1;
    }
    std::cout << "Connected!\n\n";

    uint32_t hdr[3];
    hdr[0] = htonl((uint32_t)scheme);
    hdr[1] = htonl((uint32_t)total_frames);
    hdr[2] = htonl((uint32_t)ft);
    
    if (send_all(sockfd, hdr, sizeof(hdr)) == false) {
        std::cout << "Error: failed to send header.\n"; 
        return 1;
    }

    int acks = 0;
    int nacks = 0;
    size_t offset = 0;

    auto t_start = std::chrono::high_resolution_clock::now();

    std::cout << "----------------------------------------------------------------------------------------------------------\n";
    std::cout << "Frame | Bytes | Pad | FCS Value  | Status         | Error Injected                        | Rx ACK\n";
    std::cout << "----------------------------------------------------------------------------------------------------------\n";

    for (int i = 0; i < total_frames; i++) {
        int chunk_len = PAYLOAD_SIZE;
        if (message.size() - offset < (size_t)PAYLOAD_SIZE) {
            chunk_len = message.size() - offset;
        }

        Frame f = build_frame(dest_mac, src_mac, message.data() + offset, chunk_len, scheme);
        offset += chunk_len;

        uint8_t wire[FRAME_SIZE];
        frame_to_bytes(f, wire);

        std::string status = "CLEAN         ";
        std::string error_desc = "---                                   ";
        
        if (error_plan[i] != ERR_NONE) {
            InjectionResult ir = inject_error(wire, FRAME_SIZE, error_plan[i], scheme);
            if (ir.error_injected) {
                status = "ERROR INJECTED";
                error_desc = ir.description;
                if (error_desc.length() > 37) {
                    error_desc = error_desc.substr(0, 35) + "..";
                }
                while (error_desc.length() < 37) {
                    error_desc += " ";
                }
            }
        }

        if (send_all(sockfd, wire, FRAME_SIZE) == false) {
            std::cout << "Failed to send frame " << i+1 << "\n";
            break;
        }

        char ack_buf[6];
        if (recv_all(sockfd, ack_buf, 5) == false) {
            std::cout << "Failed to receive ACK for frame " << i+1 << "\n";
            break;
        }
        ack_buf[5] = '\0';
        std::string rx_ack(ack_buf);

        if (rx_ack.find("ACK") != std::string::npos && rx_ack.find("NACK") == std::string::npos) {
            acks++;
        } else {
            nacks++;
        }

        uint32_t val = fcs_unpack(f.fcs, scheme);
        
        std::cout << std::left << std::setw(5) << (i + 1) << " | "
                  << std::setw(5) << chunk_len << " | "
                  << std::setw(3) << (PAYLOAD_SIZE - chunk_len) << " | "
                  << "0x" << std::hex << std::setfill('0') << std::setw(fcs_width_bytes(scheme)*2) << val << std::dec << std::setfill(' ') << " | "
                  << status << " | " << error_desc << " | " << rx_ack << "\n";
    }

    auto t_end = std::chrono::high_resolution_clock::now();
    double total_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();

    close(sockfd);

    std::cout << "----------------------------------------------------------------------------------------------------------\n";

    std::cout << "\n=== SENDER SUMMARY ===\n";
    std::cout << "Input file       : " << input_file << "\n";
    std::cout << "File type        : " << fileTypeName(ft) << "\n";
    std::cout << "Message bytes    : " << message.size() << "\n";
    std::cout << "Frames sent      : " << total_frames << "\n";
    std::cout << "Receiver ACKs    : " << acks << "\n";
    std::cout << "Receiver NACKs   : " << nacks << "\n";
    std::cout << "Detection scheme : " << getSchemeInfo(scheme).name << "\n";
    std::cout << "Total send time  : " << total_ms << " ms\n";
    std::cout << "======================\n\n";

    return 0;
}
