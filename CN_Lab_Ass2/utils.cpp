#include "utils.hpp"
#include <iostream>
#include <iomanip>
#include <numeric>
#include <algorithm>

using namespace std;

void print_visualization(const SimulationStats& stats) {
    cout << "\n============================================================\n";
    cout << "             DATA LINK LAYER PROTOCOL ANALYSIS              \n";
    cout << "============================================================\n";
    
    string proto = (stats.protocol == 1) ? "Stop-and-Wait ARQ" : (stats.protocol == 2) ? "Go-Back-N ARQ" : "Selective Repeat ARQ";
    cout << " Protocol           : " << proto << "\n";
    cout << " Loss Probability   : " << stats.prob_loss << "\n";
    cout << " Error Probability  : " << stats.prob_error << "\n";
    cout << "------------------------------------------------------------\n";
    cout << " Total Data Frames  : " << stats.total_frames << "\n";
    cout << " Total Transmitted  : " << stats.total_transmissions << " (Includes retransmissions)\n";
    cout << " Total Timeouts     : " << stats.total_timeouts << "\n";
    cout << " Total Time Elapsed : " << stats.elapsed_time_ms << " ms\n";
    
    if (!stats.rtt_samples.empty()) {
        double min_rtt = *min_element(stats.rtt_samples.begin(), stats.rtt_samples.end());
        double max_rtt = *max_element(stats.rtt_samples.begin(), stats.rtt_samples.end());
        double sum_rtt = accumulate(stats.rtt_samples.begin(), stats.rtt_samples.end(), 0.0);
        double avg_rtt = sum_rtt / stats.rtt_samples.size();
        cout << "------------------------------------------------------------\n";
        cout << " RTT (Propagation + ACK Reception) Analysis:\n";
        cout << " - Min RTT          : " << min_rtt << " ms\n";
        cout << " - Max RTT          : " << max_rtt << " ms\n";
        cout << " - Avg RTT          : " << avg_rtt << " ms\n";
    }
    
    double efficiency = 0;
    if (stats.total_transmissions > 0) {
        efficiency = ((double)stats.total_frames / stats.total_transmissions) * 100.0;
    }
    cout << "------------------------------------------------------------\n";
    cout << " Throughput Efficiency (Unique/Total Transmissions) : " << fixed << setprecision(2) << efficiency << " %\n";
    cout << "============================================================\n";
    
    cout << " Efficiency Visualization:\n [";
    int bars = (int)(efficiency / 2.0);
    for(int i=0; i<50; i++) {
        if(i < bars) cout << "#";
        else cout << " ";
    }
    cout << "] " << efficiency << "%\n";
    
    cout << " Packet Distribution:\n";
    int overhead_pct = 100 - (int)efficiency;
    cout << " Unique Frames : [";
    for(int i=0; i<50; i++) {
        if(i < bars) cout << "="; else cout << " ";
    }
    cout << "] " << (int)efficiency << "%\n";
    cout << " Retransmitted : [";
    for(int i=0; i<50; i++) {
        if(i < (50 - bars)) cout << "*"; else cout << " ";
    }
    cout << "] " << overhead_pct << "%\n";
    cout << "============================================================\n\n";
    cout << "CONCLUSION based on Assignment 2 conditions:\n";
    cout << "- Time between propagation and ACK is represented in Average RTT.\n";
    if (stats.prob_loss == 0 && stats.prob_error == 0) {
        cout << "- Since error/loss is 0, efficiency represents raw protocol capability without network interference.\n";
    } else {
        cout << "- Efficiency drops proportionally to the specified error (" << stats.prob_error << ") and loss (" << stats.prob_loss << ") rates.\n";
        if (stats.protocol == 2 && stats.total_timeouts > 0) {
            cout << "- Notice how Go-Back-N's efficiency drops drastically due to window retransmissions compared to Selective Repeat.\n";
        }
    }
    cout << "============================================================\n\n";
}
