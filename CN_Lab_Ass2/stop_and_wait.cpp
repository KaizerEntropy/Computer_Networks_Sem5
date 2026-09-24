#include "stop_and_wait.hpp"
#include "error.hpp"
#include <iostream>
#include <chrono>
#include <unistd.h>
#include <sys/socket.h>

using namespace std;

void run_sw_sender(int sock, struct sockaddr_in& dest_addr, const vector<Frame>& frames, int timeout_ms, double prob_loss, double prob_error, int scheme, SimulationStats& stats) {
    int current_frame = 0;
    int total_transmissions = 0;
    int total_timeouts = 0;
    socklen_t dest_len = sizeof(dest_addr);

    while (current_frame < (int)frames.size()) {
        Frame f = frames[current_frame];
        f.calculate_fcs(scheme);
        
        cout << "[SW Sender] Sending Frame " << current_frame << " (Seq: " << (int)f.seq_no << ")\n";
        
        if (channel_transmit(f, prob_loss, prob_error)) {
            vector<uint8_t> buffer = f.serialize();
            sendto(sock, buffer.data(), buffer.size(), 0, (struct sockaddr*)&dest_addr, dest_len);
        }
        total_transmissions++;
        
        auto start_time = chrono::steady_clock::now();
        bool acked = false;
        
        while (!acked) {
            uint8_t recv_buf[1024];
            int n = recvfrom(sock, recv_buf, sizeof(recv_buf), 0, NULL, NULL);
            if (n > 0) {
                vector<uint8_t> vec_buf(recv_buf, recv_buf + n);
                Frame ack_f = Frame::deserialize(vec_buf);
                if (ack_f.type == FrameType::ACK && ack_f.verify_fcs(scheme)) {
                    if (ack_f.ack_seq_no == (f.seq_no + 1) % 256) {
                        auto end_time = chrono::steady_clock::now();
                        double rtt = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();
                        stats.rtt_samples.push_back(rtt);
                        cout << "[SW Sender] Received ACK for Frame " << current_frame << " (RTT: " << rtt << " ms)\n";
                        acked = true;
                        current_frame++;
                    }
                }
            }
            
            auto current_time = chrono::steady_clock::now();
            auto elapsed = chrono::duration_cast<chrono::milliseconds>(current_time - start_time).count();
            if (elapsed > timeout_ms && !acked) {
                cout << "[SW Sender] Timeout! Retransmitting Frame " << current_frame << "\n";
                total_timeouts++;
                break;
            }
        }
    }
    
    // Send EOF
    Frame eof;
    eof.type = FrameType::DATA;
    eof.seq_no = 255;
    eof.length = 0;
    eof.calculate_fcs(scheme);
    vector<uint8_t> buffer = eof.serialize();
    sendto(sock, buffer.data(), buffer.size(), 0, (struct sockaddr*)&dest_addr, dest_len);

    stats.total_transmissions = total_transmissions;
    stats.total_timeouts = total_timeouts;
    cout << "\n=== SW Transmission Complete ===\n";
    cout << "Total Transmissions: " << total_transmissions << "\n";
    cout << "Total Timeouts: " << total_timeouts << "\n";
}

void run_sw_receiver(int sock, double prob_ack_loss, int scheme, std::vector<uint8_t>& final_data) {
    int expected_seq = 0;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (true) {
        uint8_t recv_buf[1500];
        int n = recvfrom(sock, recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)&client_addr, &client_len);
        
        if (n > 0) {
            vector<uint8_t> vec_buf(recv_buf, recv_buf + n);
            Frame f = Frame::deserialize(vec_buf);
            
            if (f.seq_no == 255 && f.length == 0) {
                cout << "\n[SW Receiver] EOF Frame received. Transmission complete.\n";
                break;
            }

            if (!f.verify_fcs(scheme)) {
                cout << "[SW Receiver] FCS Error in Frame " << (int)f.seq_no << ". Discarding.\n";
                continue;
            }

            if (f.seq_no == expected_seq) {
                cout << "[SW Receiver] Received expected Frame " << (int)f.seq_no << ". Sending ACK.\n";
                final_data.insert(final_data.end(), f.payload.begin(), f.payload.end());
                expected_seq++;
            } else {
                cout << "[SW Receiver] Received duplicate Frame " << (int)f.seq_no << ". Retransmitting ACK.\n";
            }
            
            Frame ack;
            ack.type = FrameType::ACK;
            ack.ack_seq_no = expected_seq; // Expected next seq
            ack.length = 0;
            ack.seq_no = 0;
            ack.calculate_fcs(scheme);
            
            if (channel_transmit(ack, prob_ack_loss, 0.0)) {
                vector<uint8_t> buffer = ack.serialize();
                sendto(sock, buffer.data(), buffer.size(), 0, (struct sockaddr*)&client_addr, client_len);
            }
        }
    }
}
