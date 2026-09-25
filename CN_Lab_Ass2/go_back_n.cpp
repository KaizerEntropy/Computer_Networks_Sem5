#include "go_back_n.hpp"
#include "error.hpp"
#include <iostream>
#include <chrono>
#include <unistd.h>
#include <sys/socket.h>
#include <thread>
#include <mutex>
#include <map>

using namespace std;

struct GBNState {
    int base_seq = 0;
    bool is_finished = false;
    mutex mtx;
    SimulationStats* stats = nullptr;
    map<int, chrono::time_point<chrono::steady_clock>> send_times;
};

void gbn_recv_thread(int sock, int scheme, GBNState* state, int total_frames) {
    while (true) {
        state->mtx.lock();
        if (state->is_finished) {
            state->mtx.unlock();
            break;
        }
        state->mtx.unlock();

        uint8_t recv_buf[1024];
        int n = recvfrom(sock, recv_buf, sizeof(recv_buf), 0, NULL, NULL);
        if (n > 0) {
            vector<uint8_t> vec_buf(recv_buf, recv_buf + n);
            Frame f = Frame::deserialize(vec_buf);
            if (f.type == FrameType::ACK && f.verify_fcs(scheme)) {
                state->mtx.lock();
                int acked_idx = f.ack_seq_no; 
                if (acked_idx > state->base_seq && acked_idx <= total_frames) {
                    auto end_time = chrono::steady_clock::now();
                    // We can estimate RTT for the newly acked frames
                    for (int i = state->base_seq; i < acked_idx; i++) {
                        if (state->send_times.find(i) != state->send_times.end()) {
                            double rtt = chrono::duration_cast<chrono::milliseconds>(end_time - state->send_times[i]).count();
                            if (state->stats) state->stats->rtt_samples.push_back(rtt);
                        }
                    }
                    cout << "[GBN Sender] Received Cumulative ACK up to " << acked_idx - 1 << " (Next expected: " << acked_idx << ")\n";
                    state->base_seq = acked_idx;
                }
                state->mtx.unlock();
            }
        }
    }
}

void run_gbn_sender(int sock, struct sockaddr_in& dest_addr, const vector<Frame>& frames, int window_size, int timeout_ms, double prob_loss, double prob_error, double prob_delay, int scheme, SimulationStats& stats) {
    GBNState state;
    state.stats = &stats;
    int next_seq = 0;
    int total_transmissions = 0;
    int total_timeouts = 0;
    socklen_t dest_len = sizeof(dest_addr);

    thread recv_th(gbn_recv_thread, sock, scheme, &state, frames.size());
    auto timer_start = chrono::steady_clock::now();

    while (true) {
        state.mtx.lock();
        if (state.base_seq >= (int)frames.size()) {
            state.is_finished = true;
            state.mtx.unlock();
            break;
        }
        
        while (next_seq < state.base_seq + window_size && next_seq < (int)frames.size()) {
            Frame f = frames[next_seq];
            f.calculate_fcs(scheme);
            
            cout << "[GBN Sender] Sending Frame " << next_seq << " (Seq: " << (int)f.seq_no << ")\n";
            if (channel_transmit(f, prob_loss, prob_error, prob_delay)) {
                vector<uint8_t> buffer = f.serialize();
                sendto(sock, buffer.data(), buffer.size(), 0, (struct sockaddr*)&dest_addr, dest_len);
            }
            total_transmissions++;
            
            if (state.send_times.find(next_seq) == state.send_times.end()) {
                 state.send_times[next_seq] = chrono::steady_clock::now();
            }

            if (state.base_seq == next_seq) {
                timer_start = chrono::steady_clock::now();
            }
            next_seq++;
        }
        
        auto current_time = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::milliseconds>(current_time - timer_start).count();
        if (elapsed > timeout_ms) {
            cout << "[GBN Sender] Timeout! Base: " << state.base_seq << ". Retransmitting window...\n";
            total_timeouts++;
            timer_start = chrono::steady_clock::now();
            for (int i = state.base_seq; i < next_seq; i++) {
                Frame f = frames[i];
                f.calculate_fcs(scheme);
                cout << "[GBN Sender] Retransmitting Frame " << i << " (Seq: " << (int)f.seq_no << ")\n";
                if (channel_transmit(f, prob_loss, prob_error, prob_delay)) {
                    vector<uint8_t> buffer = f.serialize();
                    sendto(sock, buffer.data(), buffer.size(), 0, (struct sockaddr*)&dest_addr, dest_len);
                }
                total_transmissions++;
            }
        }
        state.mtx.unlock();
        this_thread::sleep_for(chrono::milliseconds(5));
    }
    
    recv_th.join();

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
    cout << "\n=== GBN Transmission Complete ===\n";
    cout << "Total Transmissions: " << total_transmissions << "\n";
    cout << "Total Timeouts: " << total_timeouts << "\n";
}

void run_gbn_receiver(int sock, double prob_ack_loss, int scheme, std::vector<uint8_t>& final_data) {
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
                cout << "\n[GBN Receiver] EOF Frame received. Transmission complete.\n";
                break;
            }

            if (!f.verify_fcs(scheme)) {
                cout << "[GBN Receiver] FCS Error in Frame " << (int)f.seq_no << ". Discarding.\n";
                continue;
            }

            if (f.seq_no == expected_seq) {
                cout << "[GBN Receiver] Received expected Frame " << (int)f.seq_no << ". Sending Cumulative ACK.\n";
                final_data.insert(final_data.end(), f.payload.begin(), f.payload.end());
                expected_seq++;
            } else {
                cout << "[GBN Receiver] Out of order Frame " << (int)f.seq_no << " (Expected " << expected_seq << "). Discarding.\n";
            }

            // Always send ACK for expected_seq (which implies cumulative acking all frames before it)
            Frame ack;
            ack.type = FrameType::ACK;
            ack.ack_seq_no = expected_seq;
            ack.length = 0;
            ack.seq_no = 0;
            ack.calculate_fcs(scheme);
            
            if (channel_transmit(ack, prob_ack_loss, 0.0, 0.0)) {
                vector<uint8_t> buffer = ack.serialize();
                sendto(sock, buffer.data(), buffer.size(), 0, (struct sockaddr*)&client_addr, client_len);
            }
        }
    }
}
