#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <omp.h>

using namespace std;

// -------------------------------------------------------------
// Engine 1: Pure OpenMP Balanced Graph Coloring (Shared Memory)
// Based on Lu et al., IEEE TPDS 2017 (Greedy-FF + VFF Balancing)
// -------------------------------------------------------------

int main() {
    cout << "============================================================" << endl;
    cout << "   5G TOWER UPDATE SCHEDULER - PURE OPENMP ENGINE           " << endl;
    cout << "   (Shared-Memory Multi-Threaded Guided Balancing)          " << endl;
    cout << "============================================================" << endl;

    // 1. Read active graph
    ifstream fin("active_graph.txt");
    if (!fin.is_open()) {
        cout << "[Error] Could not open active_graph.txt!" << endl;
        return 1;
    }

    int N = 0; // Number of active towers
    int M = 0; // Number of interference edges
    fin >> N >> M;

    if (N <= 0) {
        cout << "[Warning] No active towers to schedule (N = 0)." << endl;
        return 0;
    }

    // Priority ordering of active towers
    vector<int> priority_order(N);
    for (int i = 0; i < N; i++) {
        fin >> priority_order[i];
    }

    // Adjacency list and edge list for conflict checking
    vector<vector<int>> adj(N);
    vector<int> edge_u(M);
    vector<int> edge_v(M);
    for (int i = 0; i < M; i++) {
        int u, v;
        fin >> u >> v;
        adj[u].push_back(v);
        adj[v].push_back(u);
        edge_u[i] = u;
        edge_v[i] = v;
    }
    fin.close();

    cout << "[Step 1] Loaded active graph: " << N << " towers, " 
         << M << " interference edges." << endl;

    double t_start = omp_get_wtime();

    // 2. Initial Coloring: Priority-Ordered Greedy First-Fit (Greedy-FF)
    vector<int> color(N, -1);
    for (int i = 0; i < N; i++) {
        int u = priority_order[i];
        vector<bool> forbidden(N + 1, false);
        for (int j = 0; j < (int)adj[u].size(); j++) {
            int v = adj[u][j];
            if (color[v] != -1) {
                forbidden[color[v]] = true;
            }
        }
        int c = 0;
        while (forbidden[c]) {
            c++;
        }
        color[u] = c;
    }

    // Determine number of colors C (update batches)
    int C = 0;
    for (int i = 0; i < N; i++) {
        if (color[i] + 1 > C) {
            C = color[i] + 1;
        }
    }

    // Compute initial bin sizes
    vector<int> bin_size(C, 0);
    for (int i = 0; i < N; i++) {
        bin_size[color[i]]++;
    }
    vector<int> initial_bins = bin_size;

    // Target per batch: gamma = ceil(N / C)
    int gamma = (N + C - 1) / C;

    // Compute Initial RSD%
    double mean = (double)N / C;
    double sum_sq = 0.0;
    for (int c = 0; c < C; c++) {
        sum_sq += (bin_size[c] - mean) * (bin_size[c] - mean);
    }
    double initial_rsd = (sqrt(sum_sq / C) / mean) * 100.0;

    cout << "[Step 2] Greedy First-Fit Coloring Complete:" << endl;
    cout << "         - Total Batches (Colors C): " << C << endl;
    cout << "         - Target per Batch (gamma): " << gamma << endl;
    cout << "         - Initial Bin Distribution: [";
    for (int c = 0; c < C; c++) {
        cout << bin_size[c] << (c + 1 < C ? ", " : "");
    }
    cout << "]" << endl;
    cout << fixed << setprecision(2);
    cout << "         - Initial RSD%: " << initial_rsd << "% (Skewed Load)" << endl;

    // 3. Step 2: Parallel VFF Guided Balancing (Speculation & Iteration)
    int num_threads = omp_get_max_threads();
    cout << "[Step 3] Launching Parallel VFF Balancing with " 
         << num_threads << " OpenMP threads..." << endl;

    int total_conflicts_resolved = 0;
    const int MAX_ITERS = 10;
    vector<int> speculative_bin_size = bin_size;

    for (int iter = 0; iter < MAX_ITERS; iter++) {
        vector<int> tentative_color = color;
        vector<int> old_color = color;
        speculative_bin_size = bin_size;
        int moves_proposed = 0;

        // In parallel, find vertices in over-full bins and propose moves to under-full bins
        #pragma omp parallel for reduction(+:moves_proposed)
        for (int i = 0; i < N; i++) {
            int curr_c = color[i];
            if (bin_size[curr_c] > gamma) {
                // Find forbidden colors from neighbors
                vector<bool> nbr_has_color(C, false);
                for (int j = 0; j < (int)adj[i].size(); j++) {
                    int v = adj[i][j];
                    if (color[v] >= 0 && color[v] < C) {
                        nbr_has_color[color[v]] = true;
                    }
                }

                int chosen_c = -1;
                // Critical section safely claims a slot in under-full bins
                #pragma omp critical
                {
                    int best_c = -1;
                    int min_bin_count = gamma;
                    for (int c = 0; c < C; c++) {
                        if (c != curr_c && !nbr_has_color[c] && speculative_bin_size[c] < gamma) {
                            if (speculative_bin_size[c] < min_bin_count) {
                                min_bin_count = speculative_bin_size[c];
                                best_c = c;
                            }
                        }
                    }
                    if (best_c != -1) {
                        speculative_bin_size[curr_c]--;
                        speculative_bin_size[best_c]++;
                        chosen_c = best_c;
                    }
                }

                if (chosen_c != -1) {
                    tentative_color[i] = chosen_c;
                    moves_proposed++;
                }
            }
        }

        if (moves_proposed == 0) {
            cout << "         - Iteration " << (iter + 1) 
                 << ": No further moves possible (Bins balanced)." << endl;
            break;
        }

        // Parallel Conflict Resolution using the u > v rule
        vector<bool> revert(N, false);
        int conflicts_in_iter = 0;

        #pragma omp parallel for reduction(+:conflicts_in_iter)
        for (int e = 0; e < M; e++) {
            int u = edge_u[e];
            int v = edge_v[e];
            if (tentative_color[u] == tentative_color[v]) {
                int higher_v = (u > v) ? u : v;
                revert[higher_v] = true;
                conflicts_in_iter++;
            }
        }

        // Apply reversions
        for (int i = 0; i < N; i++) {
            if (revert[i]) {
                tentative_color[i] = old_color[i];
            }
        }

        total_conflicts_resolved += conflicts_in_iter;

        // Update colors
        color = tentative_color;

        // Recalculate bin sizes
        fill(bin_size.begin(), bin_size.end(), 0);
        for (int i = 0; i < N; i++) {
            bin_size[color[i]]++;
        }

        cout << "         - Iteration " << (iter + 1) << ": "
             << moves_proposed << " moves proposed, "
             << conflicts_in_iter << " conflicts resolved." << endl;
    }

    // 4. Simulate Server Updating Batch by Batch in Parallel
    #pragma omp parallel for
    for (int c = 0; c < C; c++) {
        long long dummy_work = 0;
        for (int i = 0; i < N; i++) {
            if (color[i] == c) {
                dummy_work += 150000;
            }
        }
        for (volatile long long w = 0; w < dummy_work; w++) {
            // Simulated firmware dispatch delay per batch
        }
    }

    double t_end = omp_get_wtime();
    double total_time_ms = (t_end - t_start) * 1000.0;

    // 5. Final RSD% Calculation
    sum_sq = 0.0;
    for (int c = 0; c < C; c++) {
        sum_sq += (bin_size[c] - mean) * (bin_size[c] - mean);
    }
    double final_rsd = (sqrt(sum_sq / C) / mean) * 100.0;

    cout << "[Step 4] Schedule Optimization Complete:" << endl;
    cout << "         - Final Bin Distribution:   [";
    for (int c = 0; c < C; c++) {
        cout << bin_size[c] << (c + 1 < C ? ", " : "");
    }
    cout << "]" << endl;
    cout << "         - Final RSD%: " << final_rsd << "%" << endl;
    cout << "         - RSD Reduction: " << (initial_rsd - final_rsd) << "%" << endl;
    cout << "         - Total Conflicts Resolved: " << total_conflicts_resolved << endl;
    cout << "         - Execution Time: " << total_time_ms << " ms" << endl;
    cout << "============================================================" << endl;

    // 6. Write structured output to color_output.txt for UI consumption
    ofstream fout("color_output.txt");
    if (fout.is_open()) {
        fout << "COLORS:";
        for (int i = 0; i < N; i++) {
            fout << " " << color[i];
        }
        fout << endl;
        fout << "NUM_COLORS: " << C << endl;
        fout << "GAMMA: " << gamma << endl;
        fout << "INITIAL_RSD: " << initial_rsd << endl;
        fout << "FINAL_RSD: " << final_rsd << endl;
        fout << "CONFLICTS: " << total_conflicts_resolved << endl;
        fout << "TIME_MS: " << total_time_ms << endl;
        fout << "INITIAL_BINS:";
        for (int c = 0; c < C; c++) fout << " " << initial_bins[c];
        fout << endl;
        fout << "FINAL_BINS:";
        for (int c = 0; c < C; c++) fout << " " << bin_size[c];
        fout << endl;
        fout.close();
    }

    return 0;
}
