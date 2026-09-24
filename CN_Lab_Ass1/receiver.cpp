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
    int port = 9000;
    if (argc >= 2) {
        port = std::stoi(argv[1]);
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { 
        std::cout << "Socket creation failed.\n"; 
        return 1; 
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cout << "Bind failed.\n"; 
        return 1;
    }

    if (listen(server_fd, 1) < 0) {
        std::cout << "Listen failed.\n"; 
        return 1;
    }

    std::cout << "\n--------------------------------------------------------\n";
    std::cout <<   "     RECEIVER - Error Detection Socket Programming      \n";
    std::cout <<   "--------------------------------------------------------\n\n";

    std::cout << "Listening on port " << port << " ... waiting for sender.\n";

    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) { 
        std::cout << "Accept failed.\n"; 
        return 1; 
    }
    
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
    std::cout << "Sender connected from " << client_ip << "!\n\n";

    uint32_t hdr[3];
    if (recv_all(client_fd, hdr, sizeof(hdr)) == false) {
        std::cout << "Error: failed to read header.\n"; 
        return 1;
    }
    
    int scheme = ntohl(hdr[0]);
    int total_frames = ntohl(hdr[1]);
    FileType ft = (FileType)ntohl(hdr[2]);

    if (scheme < 1 || scheme > 5) {
        std::cout << "Error: invalid scheme.\n"; 
        return 1;
    }

    std::vector<int> error_plan = build_error_plan(total_frames);

    std::cout << "--- RECEPTION INFO ---\n";
    std::cout << "Detection scheme : " << getSchemeInfo(scheme).name << "\n";
    std::cout << "File type        : " << fileTypeName(ft) << "\n";
    std::cout << "Frames expected  : " << total_frames << "\n";
    std::cout << "Error mode       : Round-robin (7-type cycle)\n";
    std::cout << "----------------------\n\n";

    std::vector<uint8_t> reconstructed;
    int accepted_count = 0;
    int rejected_count = 0;
    std::vector<int> rejected_frames;

    double total_verify_us = 0;

    auto t_start = std::chrono::high_resolution_clock::now();

    std::cout << "--------------------------------------------------------------------------------------\n";
    std::cout << "Frame | Length | FCS (recv / calc)     | Verdict  | Expected Error Type           \n";
    std::cout << "--------------------------------------------------------------------------------------\n";

    for (int i = 0; i < total_frames; i++) {
        uint8_t wire[FRAME_SIZE];
        if (recv_all(client_fd, wire, FRAME_SIZE) == false) {
            std::cout << "Connection lost at frame " << i+1 << "\n";
            break;
        }

        Frame f = frame_from_bytes(wire);
        int chunk_len = f.length;

        uint8_t coverage[FCS_COVER_LEN];
        int off = 0;
        for(int j=0; j<DEST_LEN; j++) coverage[off+j] = f.dest[j];
        off += DEST_LEN;
        
        for(int j=0; j<SRC_LEN; j++) coverage[off+j] = f.src[j];
        off += SRC_LEN;
        
        coverage[off] = (uint8_t)((f.length >> 8) & 0xFF);
        coverage[off + 1] = (uint8_t)(f.length & 0xFF);
        off += 2;
        
        for(int j=0; j<PAYLOAD_SIZE; j++) coverage[off+j] = f.payload[j];

        auto v_start = std::chrono::high_resolution_clock::now();
        uint32_t recomputed = fcs_compute(coverage, FCS_COVER_LEN, scheme);
        auto v_end = std::chrono::high_resolution_clock::now();
        total_verify_us += std::chrono::duration<double, std::micro>(v_end - v_start).count();

        uint32_t received_fcs = fcs_unpack(f.fcs, scheme);
        bool unused_clean = fcs_unused_clean(f.fcs, scheme);

        bool accepted = false;
        if (recomputed == received_fcs && unused_clean) {
            accepted = true;
        }

        if (accepted) {
            for (int k = 0; k < chunk_len; k++) {
                reconstructed.push_back(f.payload[k]);
            }
            send_all(client_fd, "ACK  ", 5);
            accepted_count++;
        } else {
            send_all(client_fd, "NACK ", 5);
            rejected_count++;
            rejected_frames.push_back(i + 1);
        }

        std::string verdict = accepted ? "ACK  " : "NACK ";
        std::string exp_err = errorTypeName(error_plan[i]);
        
        std::cout << std::left << std::setw(5) << (i + 1) << " | "
                  << std::setw(6) << chunk_len << " | "
                  << "0x" << std::hex << std::setfill('0') << std::setw(fcs_width_bytes(scheme)*2) << received_fcs
                  << " / "
                  << "0x" << std::setw(fcs_width_bytes(scheme)*2) << recomputed << std::dec << std::setfill(' ') << " | "
                  << std::setw(8) << verdict << " | "
                  << exp_err << "\n";
    }

    auto t_end = std::chrono::high_resolution_clock::now();
    double total_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();

    close(client_fd);
    close(server_fd);

    std::cout << "--------------------------------------------------------------------------------------\n";

    std::string out_path;
    if (ft == FILE_BITS) {
        out_path = "received_output.bits";
    } else {
        out_path = "received_output.txt";
    }
    
    std::ofstream fout(out_path, std::ios::binary);
    if (ft == FILE_TEXT) {
        for (size_t i = 0; i < reconstructed.size(); i++) {
            fout.put((char)reconstructed[i]);
        }
    } else {
        std::string bs = bytes_to_bits_string(reconstructed);
        fout.write(bs.c_str(), bs.length());
    }
    fout.close();

    std::cout << "\n=== RECEIVER SUMMARY ===\n";
    std::cout << "Detection scheme    : " << getSchemeInfo(scheme).name << "\n";
    std::cout << "File type           : " << fileTypeName(ft) << "\n";
    std::cout << "Frames expected     : " << total_frames << "\n";
    std::cout << "Frames received     : " << (accepted_count + rejected_count) << "\n";
    std::cout << "Frames ACCEPTED     : " << accepted_count << "\n";
    std::cout << "Frames REJECTED     : " << rejected_count << "\n";
    
    std::cout << "Rejected frames     : ";
    if (rejected_frames.size() == 0) {
        std::cout << "None\n";
    } else {
        for (size_t i = 0; i < rejected_frames.size() && i < 10; i++) {
            std::cout << rejected_frames[i];
            if (i < rejected_frames.size() - 1 && i < 9) std::cout << ", ";
        }
        if (rejected_frames.size() > 10) std::cout << "...";
        std::cout << "\n";
    }
    
    std::cout << "Bytes reconstructed : " << reconstructed.size() << "\n";
    std::cout << "Output file         : " << out_path << "\n";
    std::cout << "Total receive time  : " << total_ms << " ms\n";
    
    double avg_verify = 0;
    if (total_frames > 0) {
        avg_verify = total_verify_us / total_frames;
    }
    std::cout << "Avg verify time     : " << avg_verify << " us\n";
    std::cout << "========================\n\n";
    
    if (rejected_count > 0) {
        std::cout << "  NOTE: Rejected frames were NOT written to the output file.\n";
        std::cout << "        The reconstructed message has gaps at those positions.\n\n";
    }

    return 0;
}
