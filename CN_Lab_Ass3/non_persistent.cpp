#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <cstdlib>
#include "csma.hpp"
#include "utils.hpp"

using namespace std;

void run_non_persistent(int sock, struct sockaddr_in& server_addr, int station_id, int frames_to_send) {
    cout << "\n[Station " << station_id << "] Starting Non-Persistent CSMA...\n";
    for (int i = 0; i < frames_to_send; ++i) {
        bool success = false;
        while (!success) {
            Message sense_msg = {MsgType::SENSE, station_id, 0};
            sendto(sock, &sense_msg, sizeof(sense_msg), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
            
            Message reply;
            int n = recvfrom(sock, &reply, sizeof(reply), 0, NULL, NULL);
            if (n > 0 && reply.type == MsgType::SENSE_REPLY_IDLE) {
                cout << "[Station " << station_id << "] Sensed IDLE. Transmitting Frame " << i << "...\n";
                Message trans_msg = {MsgType::TRANSMIT, station_id, i};
                sendto(sock, &trans_msg, sizeof(trans_msg), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
                
                while (true) {
                    recvfrom(sock, &reply, sizeof(reply), 0, NULL, NULL);
                    if (reply.type == MsgType::SUCCESS) {
                        cout << "[Station " << station_id << "] Frame " << i << " SUCCESS!\n";
                        success = true;
                        break;
                    } else if (reply.type == MsgType::COLLISION) {
                        cout << "[Station " << station_id << "] Frame " << i << " COLLISION! Backing off...\n";
                        int backoff = (rand() % 100) + 50;
                        this_thread::sleep_for(chrono::milliseconds(backoff));
                        break;
                    }
                }
            } else {
                // Busy! Do NOT keep sensing. Backoff immediately!
                int backoff = (rand() % 100) + 50;
                this_thread::sleep_for(chrono::milliseconds(backoff));
            }
        }
    }
}
