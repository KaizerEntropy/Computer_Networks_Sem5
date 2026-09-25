#pragma once
#include "frame.hpp"

// Simulates a channel. 
// Returns false if packet is lost.
// Modifies 'f' if bit error is applied.
bool channel_transmit(Frame& f, double prob_loss, double prob_error, double prob_delay);
