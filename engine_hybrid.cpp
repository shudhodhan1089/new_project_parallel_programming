#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <mpi.h>
#include <omp.h>

using namespace std;

// -------------------------------------------------------------
// Engine 3: Hybrid OpenMP + MPI Balanced Graph Coloring
// (Distributed Nodes + Multi-Threaded Shared Memory per Node)
// Based on Lu et al., IEEE TPDS 2017 (Greedy-FF + VFF Balancing)
// -------------------------------------------------------------

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank = 0;
    int size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Set 2 OpenMP threads per MPI rank as specified in Task 2.3
    omp_set_num_threads(2);

    if (rank == 0) {
        cout << "============================================================" << endl;
        cout << "   5G TOWER UPDATE SCHEDULER - HYBRID OPENMP + MPI ENGINE   " << endl;
        cout << "   (Hybrid Distributed-Shared Memory Multi-Level Balancing) " << endl;
        cout << "============================================================" << endl;
    }

    // 1. All ranks read active_graph.txt
    ifstream fin("active_graph.txt");
    if (!fin.is_open()) {
        if (rank == 0) {
            cout << "[Error] Could not open active_graph.txt!" << endl;
        }
        MPI_Finalize();
        return 1;
    }

    int N = 0; // Number of active towers
    int M = 0; // Number of interference edges
    fin >> N >> M;

    if (N <= 0) {
        if (rank == 0) {
            cout << "[Warning] No active towers to schedule (N = 0)." << endl;
        }
        MPI_Finalize();
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

    if (rank == 0) {
        cout << "[Step 1] Loaded active graph: " << N << " towers, " 
             << M << " interference edges." << endl;
        cout << "         - Running with " << size << " MPI ranks x " 
             << 2 << " OpenMP threads per rank." << endl;
    }

    double t_start = MPI_Wtime();

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

    if (rank == 0) {
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
        cout << "[Step 3] Launching Hybrid OpenMP+MPI VFF Balancing..." << endl;
    }

    // 3. Partition active vertices across MPI ranks
    int my_start = rank * (N / size);
    int my_end = (rank == size - 1) ? N : (rank + 1) * (N / size);

    int total_conflicts_resolved = 0;
    const int MAX_ITERS = 10;
    vector<int> best_color = color;
    vector<int> best_bins = bin_size;
    double best_rsd = initial_rsd;
    vector<int> local_bin_size = bin_size;

    for (int iter = 0; iter < MAX_ITERS; iter++) {
        vector<int> old_color = color;
        vector<int> local_proposals(N, -1);
        int local_moves = 0;
        local_bin_size = bin_size;

        // Inside each MPI rank, evaluate vertices in parallel using OpenMP
        #pragma omp parallel for reduction(+:local_moves)
        for (int i = my_start; i < my_end; i++) {
            int curr_c = color[i];
            if (bin_size[curr_c] > gamma) {
                vector<bool> nbr_has_color(C, false);
                for (int j = 0; j < (int)adj[i].size(); j++) {
                    int v = adj[i][j];
                    if (color[v] >= 0 && color[v] < C) {
                        nbr_has_color[color[v]] = true;
                    }
                }

                int chosen_c = -1;
                #pragma omp critical
                {
                    int best_c = -1;
                    int min_bin_count = gamma;
                    for (int step = 0; step < C; step++) {
                        int c = (step + rank * 2) % C;
                        if (c != curr_c && !nbr_has_color[c] && local_bin_size[c] < gamma) {
                            if (local_bin_size[c] < min_bin_count) {
                                min_bin_count = local_bin_size[c];
                                best_c = c;
                            }
                        }
                    }
                    if (best_c != -1) {
                        local_bin_size[curr_c]--;
                        local_bin_size[best_c]++;
                        chosen_c = best_c;
                    }
                }

                if (chosen_c != -1) {
                    local_proposals[i] = chosen_c;
                    local_moves++;
                } else {
                    local_proposals[i] = curr_c;
                }
            } else {
                local_proposals[i] = curr_c;
            }
        }

        // Global synchronization across MPI ranks
        vector<int> tentative_color(N, 0);
        MPI_Allreduce(local_proposals.data(), tentative_color.data(), N, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

        int total_moves = 0;
        MPI_Allreduce(&local_moves, &total_moves, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

        if (total_moves == 0) {
            if (rank == 0) {
                cout << "         - Iteration " << (iter + 1) 
                     << ": Bins balanced across all Hybrid workers." << endl;
            }
            break;
        }

        // Parallel conflict check inside each rank using OpenMP
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
        color = tentative_color;

        // Recalculate bin sizes
        fill(bin_size.begin(), bin_size.end(), 0);
        for (int i = 0; i < N; i++) {
            bin_size[color[i]]++;
        }

        sum_sq = 0.0;
        for (int c = 0; c < C; c++) {
            sum_sq += (bin_size[c] - mean) * (bin_size[c] - mean);
        }
        double current_rsd = (sqrt(sum_sq / C) / mean) * 100.0;

        if (current_rsd < best_rsd) {
            best_rsd = current_rsd;
            best_color = color;
            best_bins = bin_size;
        } else if (iter >= 3) {
            break;
        }

        if (rank == 0) {
            cout << "         - Iteration " << (iter + 1) << ": "
                 << total_moves << " moves across nodes, "
                 << conflicts_in_iter << " conflicts resolved." << endl;
        }
    }

    color = best_color;
    bin_size = best_bins;

    // 4. Simulate batch update using OpenMP inside each MPI rank
    #pragma omp parallel for
    for (int c = rank; c < C; c += size) {
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
    MPI_Barrier(MPI_COMM_WORLD);

    double t_end = MPI_Wtime();
    double total_time_ms = (t_end - t_start) * 1000.0;
    double final_rsd = best_rsd;

    if (rank == 0) {
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

        // 6. Write structured output to color_output.txt
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
    }

    MPI_Finalize();
    return 0;
}
