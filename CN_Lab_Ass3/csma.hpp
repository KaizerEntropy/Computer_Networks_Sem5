#pragma once
#include <netinet/in.h>

void run_1_persistent(int sock, struct sockaddr_in& server_addr, int station_id, int frames_to_send);
void run_non_persistent(int sock, struct sockaddr_in& server_addr, int station_id, int frames_to_send);
void run_p_persistent(int sock, struct sockaddr_in& server_addr, int station_id, int frames_to_send, double p);
