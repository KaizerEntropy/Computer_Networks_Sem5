#pragma once
#include "frame.hpp"
#include "utils.hpp"
#include <vector>
#include <netinet/in.h>

void run_sr_sender(int sock, struct sockaddr_in& dest_addr, const std::vector<Frame>& frames, int window_size, int timeout_ms, double prob_loss, double prob_error, double prob_delay, int scheme, SimulationStats& stats);
void run_sr_receiver(int sock, int window_size, double prob_ack_loss, int scheme, std::vector<uint8_t>& final_data);
