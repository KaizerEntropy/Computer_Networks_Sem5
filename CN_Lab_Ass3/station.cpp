#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <vector>
#include <cstdlib>
#include <ctime>
#include "utils.hpp"
#include "csma.hpp"

using namespace std;

int main() {
    srand(time(0) ^ getpid()); // Random seed unique per process
    
    int station_id;
    int protocol;
    int frames_to_send = 5;
    double p = 0.5;

    cout << "Enter Station ID (e.g. 1, 2, 3...): ";
    cin >> station_id;
    
    cout << "Select CSMA Protocol:\n1. 1-Persistent\n2. Non-Persistent\n3. p-Persistent\nChoice: ";
    cin >> protocol;
    
    if (protocol == 3) {
        cout << "Enter Probability 'p' (0.0 - 1.0): ";
        cin >> p;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (protocol == 1) {
        run_1_persistent(sock, server_addr, station_id, frames_to_send);
    } else if (protocol == 2) {
        run_non_persistent(sock, server_addr, station_id, frames_to_send);
    } else if (protocol == 3) {
        run_p_persistent(sock, server_addr, station_id, frames_to_send, p);
    }
    
    cout << "\n[Station " << station_id << "] Finished transmitting all frames.\n";
    close(sock);
    return 0;
}
