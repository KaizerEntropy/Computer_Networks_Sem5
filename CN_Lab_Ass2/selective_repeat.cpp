#include "selective_repeat.hpp"
#include "error.hpp"
#include <iostream>
#include <chrono>
#include <unistd.h>
#include <sys/socket.h>
#include <thread>
#include <mutex>
#include <map>

using namespace std;

struct SRState {
    int base_seq = 0;
    bool is_finished = false;
    map<int, bool> acked_frames;
    mutex mtx;
    SimulationStats* stats = nullptr;
    map<int, chrono::time_point<chrono::steady_clock>> send_times;
};

void sr_recv_thread(int sock, int scheme, SRState* state, int total_frames, struct sockaddr_in& dest_addr, const vector<Frame>& frames, double prob_loss, double prob_error) {
    socklen_t dest_len = sizeof(dest_addr);
    
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
            
            if (f.type == FrameType::NAK && f.verify_fcs(scheme)) {
                state->mtx.lock();
                int nak_seq = f.ack_seq_no;
                if (!state->acked_frames[nak_seq]) {
                     cout << "[SR Sender] Received NAK for Frame " << nak_seq << ". Retransmitting immediately.\n";
                     Frame ret_f = frames[nak_seq];
                     ret_f.calculate_fcs(scheme);
                     if (channel_transmit(ret_f, prob_loss, prob_error)) {
                         vector<uint8_t> buffer = ret_f.serialize();
                         sendto(sock, buffer.data(), buffer.size(), 0, (struct sockaddr*)&dest_addr, dest_len);
                     }
                }
                state->mtx.unlock();
                continue;
            }

            if (f.type == FrameType::ACK && f.verify_fcs(scheme)) {
                state->mtx.lock();
                int acked_seq = f.ack_seq_no;
                if (!state->acked_frames[acked_seq]) {
                    auto end_time = chrono::steady_clock::now();
                    if (state->send_times.find(acked_seq) != state->send_times.end()) {
                        double rtt = chrono::duration_cast<chrono::milliseconds>(end_time - state->send_times[acked_seq]).count();
                        if (state->stats) state->stats->rtt_samples.push_back(rtt);
                        cout << "[SR Sender] Received Independent ACK for Frame " << acked_seq << " (RTT: " << rtt << " ms)\n";
                    } else {
                        cout << "[SR Sender] Received Independent ACK for Frame " << acked_seq << "\n";
                    }

                    state->acked_frames[acked_seq] = true;
                    
                    while (state->base_seq < total_frames && state->acked_frames[state->base_seq]) {
                        state->base_seq++;
                    }
                }
                state->mtx.unlock();
            }
        }
    }
}

void run_sr_sender(int sock, struct sockaddr_in& dest_addr, const vector<Frame>& frames, int window_size, int timeout_ms, double prob_loss, double prob_error, int scheme, SimulationStats& stats) {
    SRState state;
    state.stats = &stats;
    int next_seq = 0;
    int total_transmissions = 0;
    int total_timeouts = 0;
    socklen_t dest_len = sizeof(dest_addr);
    map<int, chrono::time_point<chrono::steady_clock>> timers;

    thread recv_th(sr_recv_thread, sock, scheme, &state, frames.size(), std::ref(dest_addr), std::ref(frames), prob_loss, prob_error);

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
            
            cout << "[SR Sender] Sending Frame " << next_seq << " (Seq: " << (int)f.seq_no << ")\n";
            if (channel_transmit(f, prob_loss, prob_error)) {
                vector<uint8_t> buffer = f.serialize();
                sendto(sock, buffer.data(), buffer.size(), 0, (struct sockaddr*)&dest_addr, dest_len);
            }
            total_transmissions++;
            
            if (state.send_times.find(next_seq) == state.send_times.end()) {
                 state.send_times[next_seq] = chrono::steady_clock::now();
            }

            timers[next_seq] = chrono::steady_clock::now();
            next_seq++;
        }
        
        auto current_time = chrono::steady_clock::now();
        for (int i = state.base_seq; i < next_seq; i++) {
            if (!state.acked_frames[i]) {
                auto elapsed = chrono::duration_cast<chrono::milliseconds>(current_time - timers[i]).count();
                if (elapsed > timeout_ms) {
                    cout << "[SR Sender] Timeout for Frame " << i << ". Retransmitting...\n";
                    total_timeouts++;
                    
                    Frame f = frames[i];
                    f.calculate_fcs(scheme);
                    if (channel_transmit(f, prob_loss, prob_error)) {
                        vector<uint8_t> buffer = f.serialize();
                        sendto(sock, buffer.data(), buffer.size(), 0, (struct sockaddr*)&dest_addr, dest_len);
                    }
                    total_transmissions++;
                    timers[i] = chrono::steady_clock::now();
                }
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
    cout << "\n=== SR Transmission Complete ===\n";
    cout << "Total Transmissions: " << total_transmissions << "\n";
    cout << "Total Timeouts: " << total_timeouts << "\n";
}

void run_sr_receiver(int sock, int window_size, double prob_ack_loss, int scheme, std::vector<uint8_t>& final_data) {
    int expected_seq = 0;
    map<int, Frame> receive_buffer;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    auto send_ack = [&](int ack_seq, FrameType type) {
        Frame ack;
        ack.type = type;
        ack.ack_seq_no = ack_seq;
        ack.length = 0;
        ack.seq_no = 0;
        ack.calculate_fcs(scheme);
        
        if (channel_transmit(ack, prob_ack_loss, 0.0)) {
            vector<uint8_t> buffer = ack.serialize();
            sendto(sock, buffer.data(), buffer.size(), 0, (struct sockaddr*)&client_addr, client_len);
        }
    };

    while (true) {
        uint8_t recv_buf[1500];
        int n = recvfrom(sock, recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)&client_addr, &client_len);
        
        if (n > 0) {
            vector<uint8_t> vec_buf(recv_buf, recv_buf + n);
            Frame f = Frame::deserialize(vec_buf);
            
            if (f.seq_no == 255 && f.length == 0) {
                cout << "\n[SR Receiver] EOF Frame received. Transmission complete.\n";
                break;
            }

            if (!f.verify_fcs(scheme)) {
                cout << "[SR Receiver] FCS Error in Frame " << (int)f.seq_no << ". Discarding.\n";
                continue;
            }

            if (f.seq_no >= expected_seq && f.seq_no < expected_seq + window_size) {
                if (receive_buffer.find(f.seq_no) == receive_buffer.end()) {
                    cout << "[SR Receiver] Received Frame " << (int)f.seq_no << ". Buffering and sending Independent ACK.\n";
                    receive_buffer[f.seq_no] = f;
                    send_ack(f.seq_no, FrameType::ACK);
                    
                    while (receive_buffer.find(expected_seq) != receive_buffer.end()) {
                        final_data.insert(final_data.end(), receive_buffer[expected_seq].payload.begin(), receive_buffer[expected_seq].payload.end());
                        receive_buffer.erase(expected_seq);
                        expected_seq++;
                    }
                } else {
                    cout << "[SR Receiver] Received duplicate Frame " << (int)f.seq_no << ". Retransmitting ACK.\n";
                    send_ack(f.seq_no, FrameType::ACK);
                }
            } else if (f.seq_no < expected_seq) {
                cout << "[SR Receiver] Received duplicate Frame " << (int)f.seq_no << ". Retransmitting ACK.\n";
                send_ack(f.seq_no, FrameType::ACK);
            } else {
                 cout << "[SR Receiver] Out of window Frame " << (int)f.seq_no << ". Discarding.\n";
            }
            
            // Trigger NAKs
            if (f.seq_no > expected_seq) {
                for (int i = expected_seq; i < f.seq_no; i++) {
                    if (receive_buffer.find(i) == receive_buffer.end()) {
                        cout << "[SR Receiver] Sending NAK for missing Frame " << i << "\n";
                        send_ack(i, FrameType::NAK);
                    }
                }
            }
        }
    }
}
