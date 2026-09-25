#include "channel.hpp"
#include "station.hpp"
#include <iostream>
#include <vector>
#include <thread>
#include <iomanip>
#include <string>
#include <fstream>

using namespace std;

void run_simulation(CSMAScheme scheme, int num_stations, int frames_per_station, double p_prob, const string& scheme_name, ofstream& csv_file) {
    Channel channel;
    vector<Station> stations;
    vector<thread> threads;
    
    // Initialize stations
    for (int i = 0; i < num_stations; ++i) {
        stations.emplace_back(i, channel, scheme, frames_per_station, p_prob);
    }
    
    // Start simulation
    auto sim_start = chrono::steady_clock::now();
    for (int i = 0; i < num_stations; ++i) {
        threads.emplace_back(&Station::run, &stations[i]);
    }
    
    // Wait for all to finish
    for (auto& t : threads) {
        t.join();
    }
    auto sim_end = chrono::steady_clock::now();
    
    // Aggregate stats
    int total_collisions = 0;
    double total_delay = 0.0;
    int total_frames = num_stations * frames_per_station;
    
    for (const auto& s : stations) {
        StationStats st = s.get_stats();
        total_collisions += st.total_collisions;
        total_delay += st.total_delay_ms;
    }
    
    double avg_delay = total_delay / total_frames;
    double sim_time_seconds = chrono::duration_cast<chrono::milliseconds>(sim_end - sim_start).count() / 1000.0;
    double throughput = (total_frames * 1000.0) / (sim_time_seconds * 1024.0 * 1024.0); // Assuming 1KB frames for throughput calc, roughly frames/sec scaled
    double normalized_throughput = (double)total_frames / (total_frames + total_collisions);

    cout << left << setw(20) << scheme_name 
         << setw(10) << num_stations 
         << setw(15) << (p_prob >= 0 ? to_string(p_prob) : "N/A") 
         << setw(15) << total_collisions 
         << setw(20) << fixed << setprecision(2) << avg_delay 
         << setw(15) << fixed << setprecision(4) << normalized_throughput << "\n";
         
    csv_file << scheme_name << "," << num_stations << "," << (p_prob >= 0 ? p_prob : 0.0) << "," 
             << total_collisions << "," << avg_delay << "," << normalized_throughput << "\n";
}

int main() {
    int frames_per_station = 50; // Increased to 50 for more statistically significant simulation
    
    ofstream csv_file("simulation_results.csv");
    csv_file << "Protocol,Stations,Probability,Collisions,AvgDelay,Throughput\n";
    
    cout << "=========================================================================================\n";
    cout << "                         CSMA PROTOCOL SIMULATION BENCHMARK                              \n";
    cout << "=========================================================================================\n";
    cout << left << setw(20) << "Protocol" 
         << setw(10) << "Stations" 
         << setw(15) << "Probability" 
         << setw(15) << "Collisions" 
         << setw(20) << "Avg Delay (ms)" 
         << setw(15) << "Throughput (S)\n";
    cout << "-----------------------------------------------------------------------------------------\n";

    // Test Case 1: p-persistent varying p (Fixed N = 10)
    int fixed_N = 10;
    vector<double> p_values = {0.1, 0.3, 0.5, 0.7, 0.9};
    for (double p : p_values) {
        run_simulation(CSMAScheme::P_PERSISTENT, fixed_N, frames_per_station, p, "p-Persistent", csv_file);
    }
    cout << "-----------------------------------------------------------------------------------------\n";

    // Test Case 2: All schemes varying N (Stations)
    vector<int> N_values = {5, 10, 15, 20};
    double optimal_p = 0.1; // Best p from typical observations

    for (int N : N_values) {
        run_simulation(CSMAScheme::NON_PERSISTENT, N, frames_per_station, -1.0, "Non-Persistent", csv_file);
        run_simulation(CSMAScheme::ONE_PERSISTENT, N, frames_per_station, -1.0, "1-Persistent", csv_file);
        run_simulation(CSMAScheme::P_PERSISTENT, N, frames_per_station, optimal_p, "p-Persistent_Opt", csv_file);
        run_simulation(CSMAScheme::CSMA_CD, N, frames_per_station, -1.0, "CSMA/CD", csv_file);
        cout << "-----------------------------------------------------------------------------------------\n";
    }

    csv_file.close();
    return 0;
}
