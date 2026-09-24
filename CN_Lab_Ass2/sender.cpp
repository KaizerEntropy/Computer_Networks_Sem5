#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "frame.hpp"
#include "stop_and_wait.hpp"
#include "go_back_n.hpp"
#include "selective_repeat.hpp"
#include "utils.hpp"
#include <chrono>
#include <thread>

using namespace std;

int main(int argc, char* argv[]) {
    srand(time(0));
    
    string receiver_ip;
    int port;
    string input_file;

    if (argc >= 4) {
        receiver_ip = argv[1];
        port = stoi(argv[2]);
        input_file = argv[3];
    } else {
        cout << "Enter Receiver IP: ";
        cin >> receiver_ip;
        cout << "Enter Port: ";
        cin >> port;
        cout << "Enter Input File: ";
        cin >> input_file;
    }

    int protocol = 1;
    cout << "Select Protocol:\n1. Stop and Wait\n2. Go-Back-N\n3. Selective Repeat\nChoice: ";
    cin >> protocol;

    int window_size = 1;
    if (protocol == 2 || protocol == 3) {
        cout << "Enter Window Size (N): ";
        cin >> window_size;
    }

    int scheme = 1;
    cout << "Select Error Checking Scheme:\n1. Checksum\n2. CRC-32\nChoice: ";
    cin >> scheme;

    double prob_loss = 0.0, prob_error = 0.0, prob_ack_loss = 0.0;
    cout << "Enter Probability of Packet Loss (0.0 - 1.0): ";
    cin >> prob_loss;
    cout << "Enter Probability of Bit Error (0.0 - 1.0): ";
    cin >> prob_error;
    cout << "Enter Probability of Receiver ACK Loss (0.0 - 1.0): ";
    cin >> prob_ack_loss;

    // Read input file
    ifstream file(input_file, ios::binary);
    if (!file) {
        cerr << "Could not open file: " << input_file << "\n";
        return 1;
    }
    
    vector<uint8_t> file_data((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    
    // Framing
    vector<Frame> frames;
    int seq = 0;
    for (size_t i = 0; i < file_data.size(); i += MIN_PAYLOAD) {
        Frame f;
        memset(f.src_mac, 0xAA, 6);
        memset(f.dest_mac, 0xBB, 6);
        f.type = FrameType::DATA;
        f.seq_no = seq++;
        f.ack_seq_no = 0;
        
        size_t chunk = min((size_t)MIN_PAYLOAD, file_data.size() - i);
        f.payload.assign(file_data.begin() + i, file_data.begin() + i + chunk);
        f.length = f.payload.size();
        frames.push_back(f);
    }
    
    cout << "\nTotal Frames to send: " << frames.size() << "\n\n";

    // Socket setup
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    inet_pton(AF_INET, receiver_ip.c_str(), &dest_addr.sin_addr);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 10000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // Reliable Setup Phase
    cout << "\n[Sender] Handshaking with Receiver to sync configurations...\n";
    Frame setup_frame;
    memset(setup_frame.src_mac, 0xAA, 6);
    memset(setup_frame.dest_mac, 0xBB, 6);
    setup_frame.type = FrameType::SETUP;
    setup_frame.seq_no = 0;
    setup_frame.ack_seq_no = 0;
    
    string config_payload = to_string(protocol) + "," + to_string(window_size) + "," + to_string(scheme) + "," + to_string(prob_ack_loss);
    setup_frame.payload.assign(config_payload.begin(), config_payload.end());
    setup_frame.length = setup_frame.payload.size();
    setup_frame.calculate_fcs(scheme);

    bool setup_acked = false;
    socklen_t dest_len = sizeof(dest_addr);
    while (!setup_acked) {
        vector<uint8_t> buffer = setup_frame.serialize();
        sendto(sock, buffer.data(), buffer.size(), 0, (struct sockaddr*)&dest_addr, dest_len);
        
        uint8_t recv_buf[1024];
        int n = recvfrom(sock, recv_buf, sizeof(recv_buf), 0, NULL, NULL);
        if (n > 0) {
            vector<uint8_t> vec_buf(recv_buf, recv_buf + n);
            Frame ack_f = Frame::deserialize(vec_buf);
            if (ack_f.type == FrameType::ACK && ack_f.verify_fcs(scheme)) {
                cout << "[Sender] Configuration synced successfully!\n\n";
                setup_acked = true;
            }
        } else {
            this_thread::sleep_for(chrono::milliseconds(100)); // Retransmit delay
        }
    }

    auto start = chrono::steady_clock::now();

    SimulationStats stats;
    stats.total_frames = frames.size();
    stats.prob_loss = prob_loss;
    stats.prob_error = prob_error;
    stats.protocol = protocol;

    if (protocol == 1) {
        run_sw_sender(sock, dest_addr, frames, 500, prob_loss, prob_error, scheme, stats);
    } else if (protocol == 2) {
        run_gbn_sender(sock, dest_addr, frames, window_size, 500, prob_loss, prob_error, scheme, stats);
    } else if (protocol == 3) {
        run_sr_sender(sock, dest_addr, frames, window_size, 500, prob_loss, prob_error, scheme, stats);
    }

    auto end = chrono::steady_clock::now();
    stats.elapsed_time_ms = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    
    print_visualization(stats);
    
    close(sock);
    return 0;
}
