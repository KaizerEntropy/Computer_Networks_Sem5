#include <iostream>
#include <fstream>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "frame.hpp"
#include "stop_and_wait.hpp"
#include "go_back_n.hpp"
#include "selective_repeat.hpp"

using namespace std;

int main(int argc, char* argv[]) {
    srand(time(0));
    
    int port;
    if (argc >= 2) {
        port = stoi(argv[1]);
    } else {
        cout << "Enter Port: ";
        cin >> port;
    }

    int protocol = 1;
    int window_size = 1;
    int scheme = 1;
    double prob_ack_loss = 0.0;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));

    cout << "\nReceiver listening on port " << port << "... waiting for sender config.\n";

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    bool configured = false;

    while (!configured) {
        uint8_t recv_buf[1024];
        int n = recvfrom(sock, recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)&client_addr, &client_len);
        if (n > 0) {
            vector<uint8_t> vec_buf(recv_buf, recv_buf + n);
            Frame f = Frame::deserialize(vec_buf);
            
            // Assume scheme 1 initially just to verify the basic header/type, but SETUP FCS uses the actual scheme
            // Wait, we don't know the scheme until we parse it! We can just parse the payload first, then verify.
            if (f.type == FrameType::SETUP) {
                string payload(f.payload.begin(), f.payload.end());
                size_t pos1 = payload.find(',');
                size_t pos2 = payload.find(',', pos1 + 1);
                size_t pos3 = payload.find(',', pos2 + 1);
                
                protocol = stoi(payload.substr(0, pos1));
                window_size = stoi(payload.substr(pos1 + 1, pos2 - pos1 - 1));
                scheme = stoi(payload.substr(pos2 + 1, pos3 - pos2 - 1));
                prob_ack_loss = stod(payload.substr(pos3 + 1));

                cout << "[Receiver] Configuration synced from Sender:\n";
                cout << "  - Protocol: " << protocol << "\n";
                cout << "  - Window Size: " << window_size << "\n";
                cout << "  - Error Scheme: " << scheme << "\n";
                cout << "  - ACK Loss Prob: " << prob_ack_loss << "\n\n";

                // Send ACK for SETUP
                Frame ack;
                ack.type = FrameType::ACK;
                ack.ack_seq_no = 0;
                ack.length = 0;
                ack.seq_no = 0;
                ack.calculate_fcs(scheme);
                
                vector<uint8_t> buffer = ack.serialize();
                sendto(sock, buffer.data(), buffer.size(), 0, (struct sockaddr*)&client_addr, client_len);
                configured = true;
            }
        }
    }

    vector<uint8_t> final_data;

    if (protocol == 1) {
        run_sw_receiver(sock, prob_ack_loss, scheme, final_data);
    } else if (protocol == 2) {
        run_gbn_receiver(sock, prob_ack_loss, scheme, final_data);
    } else if (protocol == 3) {
        run_sr_receiver(sock, window_size, prob_ack_loss, scheme, final_data);
    }
    
    // Save received file
    ofstream outfile("received_file.txt", ios::binary);
    outfile.write((char*)final_data.data(), final_data.size());
    outfile.close();
    cout << "Received data saved to 'received_file.txt' (" << final_data.size() << " bytes).\n";
    
    close(sock);
    return 0;
}
