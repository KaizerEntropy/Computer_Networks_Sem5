#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <map>
#include <set>
#include <chrono>
#include <vector>
#include "utils.hpp"

using namespace std;

int main() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));

    cout << "========================================\n";
    cout << "   CSMA CHANNEL COORDINATOR (UDP 8080)  \n";
    cout << "========================================\n";
    cout << "[Channel] Simulating shared medium...\n\n";

    bool is_busy = false;
    bool collision_occurred = false;
    set<struct sockaddr_in, AddrComparator> transmitters; 
    auto trans_start_time = chrono::steady_clock::now();
    const int TRANS_TIME_MS = 50;

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 5000; // 5ms timeout
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while (true) {
        if (is_busy) {
            auto now = chrono::steady_clock::now();
            if (chrono::duration_cast<chrono::milliseconds>(now - trans_start_time).count() >= TRANS_TIME_MS) {
                Message reply;
                if (collision_occurred) {
                    cout << "[Channel] Transmission window over. Result: COLLISION!\n\n";
                    reply.type = MsgType::COLLISION;
                } else {
                    cout << "[Channel] Transmission window over. Result: SUCCESS!\n\n";
                    reply.type = MsgType::SUCCESS;
                }
                
                for (auto& addr : transmitters) {
                    sendto(sock, &reply, sizeof(reply), 0, (struct sockaddr*)&addr, sizeof(addr));
                }
                
                is_busy = false;
                collision_occurred = false;
                transmitters.clear();
            }
        }

        Message msg;
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int n = recvfrom(sock, &msg, sizeof(msg), 0, (struct sockaddr*)&client_addr, &client_len);
        
        if (n > 0) {
            if (msg.type == MsgType::SENSE) {
                Message reply;
                reply.type = is_busy ? MsgType::SENSE_REPLY_BUSY : MsgType::SENSE_REPLY_IDLE;
                sendto(sock, &reply, sizeof(reply), 0, (struct sockaddr*)&client_addr, client_len);
            } else if (msg.type == MsgType::TRANSMIT) {
                cout << "[Channel] Station " << msg.station_id << " began transmitting Frame " << msg.data << "!\n";
                if (!is_busy) {
                    is_busy = true;
                    collision_occurred = false;
                    transmitters.insert(client_addr);
                    trans_start_time = chrono::steady_clock::now();
                } else {
                    collision_occurred = true;
                    transmitters.insert(client_addr);
                    cout << "[Channel] OVERLAP DETECTED! Collision guaranteed!\n";
                }
            }
        }
    }
    return 0;
}
