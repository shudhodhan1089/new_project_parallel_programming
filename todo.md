# Project Task Specification: 5G Cell Tower Update Scheduler (Balanced Graph Coloring)

You are building a college Parallel Programming lab mini-project based on the paper *"Algorithms for Balanced Graph Colorings with Applications in Parallel Computing"* (Lu et al., IEEE TPDS 2017)[cite: 5, 7] and our project proposal (*OpenMP-Accelerated Balanced Graph Coloring*)[cite: 1]. 

**CRITICAL STYLE RULES:**
1. **Beginner / Student C++ Style ONLY:** 
   - MUST use `using namespace std;` at the top of every `.cpp` file.
   - Use basic includes (`#include <iostream>`, `#include <vector>`, `#include <fstream>`, `#include <cmath>`, `#include <algorithm>`, `#include <omp.h>`, `#include <mpi.h>`).
   - Use simple `vector<vector<int>> adj;` and simple arrays/vectors like `vector<int> color;`.
   - DO NOT use classes, structs with constructors, templates, `std::atomic`, `alignas`, lambda functions, or pointers. Keep the code flat, readable, and well-commented like a student learning C++, OpenMP, and MPI.
2. **100% Offline Hardcoded Pune Dataset (NO APIs):**
   - Do NOT use any internet APIs or external map libraries.
   - Hardcode a realistic list of **45 cell towers across real Pune neighborhoods** (Shivajinagar, Deccan, Kothrud, Hinjewadi, Wakad, Baner, Viman Nagar, Kalyani Nagar, Hadapsar, Swargate, Katraj, Camp, Pimpri, Chinchwad) with real approximate $(X, Y)$ canvas coordinates (or Lat/Lon mapped to canvas), carrier names (`JIO`, `AIRTEL`, `VI`, `BSNL`), `user_load`, and `urgency_level`.
3. **Visual Style:** 
   - Build a modern dark-themed Python GUI (`newui.py`) using `ttkbootstrap` (`darkly` theme)[cite: 25].
   - Left side: Controls, Dropdowns, Green Summary labels, and a Bottom Comparison Output Box[cite: 25].
   - Right side: A dark-green radar map canvas (`#142b23`) showing Pune city sectors, simple road/river background lines, overlapping tower edges, and colored tower pins[cite: 24].
4. **Dual Output:** 
   - Print detailed execution logs to the **Terminal (`cout`)** AND display the parsed summary and comparison table inside the **UI Comparison Box**[cite: 25].

---

## Master TODO Checklist

### Phase 1: Hardcoded Offline Pune Dataset (Inside `newui.py`)
- [x] **Task 1.1:** Inside `newui.py`, create a hardcoded Python list `PUNE_TOWERS` containing **45 cell towers** spread across Pune neighborhoods:
  - Dense central clusters (Shivajinagar, Deccan, Camp, Swargate, Kothrud) so that many towers overlap and force `Greedy-FF` to create a skewed color distribution.
  - Moderate outer clusters (Hinjewadi, Baner, Wakad, Viman Nagar, Hadapsar, Katraj, Pimpri).
- [x] **Task 1.2:** Each tower dictionary in `PUNE_TOWERS` must have:
  - `id`: `0` to `44`
  - `area`: e.g., `"Shivajinagar"`, `"Hinjewadi"`
  - `carrier`: `"JIO"`, `"AIRTEL"`, `"VI"`, or `"BSNL"`
  - `x`, `y`: Canvas coordinates (ranging from `x: 60..840`, `y: 60..640`)
  - `needs_update`: `1` (Needs Update) or `0` (Up-to-Date / Skip)
  - `urgency`: `1` (Normal), `2` (High), `3` (Critical Security Patch)
  - `users`: Integer between `300` and `5000`
- [x] **Task 1.3:** Automatically compute interference edges between any two towers $(u, v)$ if their Euclidean canvas distance $\sqrt{(x_1-x_2)^2 + (y_1-y_2)^2} \le 135\text{ pixels}$.
- [x] **Task 1.4:** Whenever the user clicks **Run Coloring**, filter only the towers where `needs_update == 1`, map them to active indices `0 .. N-1`, write their priority order and adjacency edges into a simple text file `active_graph.txt`:
  - Line 1: `N M` (Number of active towers, Number of active edges)
  - Line 2: `p_0 p_1 ... p_{N-1}` (Vertex IDs sorted by the selected Priority Rule)
  - Next `M` lines: `u v` (Undirected edge between active tower `u` and `v`)

---

### Phase 2: The 3 Student-Style C++ Parallel Engines

All 3 engines must read `active_graph.txt`, run **Step 1: Priority-Ordered Greedy First-Fit (`Greedy-FF`)** to get initial skewed colors $C$ and Initial RSD%[cite: 2, 3, 4], then run **Step 2: Vertex-Centric First-Fit (`VFF`) Guided Balancing** (moving vertices from over-full bins $> \gamma$ to under-full bins $< \gamma$ where $\gamma = N / C$ using Speculation-and-Iteration conflict resolution)[cite: 2, 3, 11], and write final colors to `color_output.txt`.

- [x] **Task 2.1: Engine 1 — Pure OpenMP (`engine_omp.cpp`)**
  - Use `using namespace std;` and `#include <omp.h>`[cite: 3].
  - Read `active_graph.txt` using `ifstream`.
  - Run Initial Coloring (`Greedy-FF`) in priority order[cite: 3, 9]. Calculate initial `bin_size[]` and Initial RSD%[cite: 4, 16].
  - Run Parallel `VFF` Balancing using `#pragma omp parallel for`[cite: 3, 11]:
    - Calculate target $\gamma = \text{ceil}((double)N / C)$[cite: 3, 10].
    - In parallel, inspect vertices in over-full bins (`bin_size[c] > gamma`) and move them to the smallest valid under-full bin (`bin_size[k] < gamma`)[cite: 3, 11].
    - Use `#pragma omp critical` or `#pragma omp atomic` to safely update `bin_size`[cite: 3, 4].
    - Check for adjacent vertices that accidentally moved to the same color (`color[u] == color[v]` and `u > v`) and resolve conflicts iteratively[cite: 3, 11].
  - Run a dummy parallel loop simulating the server updating each color batch to measure total update time[cite: 2, 4].
  - Print clear beginner-friendly logs to `cout` (Terminal) and save final colors to `color_output.txt`.

- [x] **Task 2.2: Engine 2 — Pure MPI (`engine_mpi.cpp`)**
  - Use `using namespace std;` and `#include <mpi.h>`.
  - Initialize MPI (`MPI_Init`, `MPI_Comm_rank`, `MPI_Comm_size`).
  - All ranks read `active_graph.txt` (keeps code super simple and short for a student project!).
  - Divide active vertices across ranks (`my_start` to `my_end`).
  - Perform initial `Greedy-FF` and compute $\gamma = N / C$[cite: 3].
  - In the `VFF` balancing loop:
    - Each MPI rank attempts to move over-full vertices in its range `[my_start, my_end)` into under-full bins[cite: 3, 11].
    - Synchronize colors across all ranks using `MPI_Allreduce` or `MPI_Allgather`.
    - Check and resolve conflicts (`u > v` rule)[cite: 11] and recompute `bin_size[]`.
  - Rank 0 prints timing and RSD% metrics to `cout` and writes `color_output.txt`[cite: 4].

- [x] **Task 2.3: Engine 3 — Hybrid OpenMP + MPI (`engine_hybrid.cpp`)**
  - Use `using namespace std;`, `#include <mpi.h>`, and `#include <omp.h>`.
  - Initialize with `MPI_Init(&argc, &argv)` and set `omp_set_num_threads(2)`.
  - Divide vertices across MPI ranks, and inside each MPI rank use `#pragma omp parallel for` to evaluate vertex moves and conflict checks in parallel[cite: 3, 11].
  - Synchronize across MPI ranks using `MPI_Allreduce`.
  - Rank 0 prints metrics to `cout` and writes `color_output.txt`.

---

### Phase 3: Modern `ttkbootstrap` GUI (`newui.py`)

- [x] **Task 3.1: Window & Theme Setup**
  - Use `ttkbootstrap` (`themename="darkly"`)[cite: 25].
  - Window Title: `"5G Tower Update Scheduler - Balanced Graph Coloring (Pune Map)"`[cite: 25].
  - Left Control Panel (width 430px) + Right Radar Map Canvas (`#142b23`)[cite: 24, 25].

- [x] **Task 3.2: Left Sidebar Controls & Dropdowns**
  - **Dropdown 1 (`Select Parallel Engine`):**
    - `OpenMP Engine (Shared Memory)`
    - `MPI Engine (Distributed Processes)`
    - `Hybrid Engine (OpenMP + MPI)`
    - `Compare All 3 Engines`
  - **Dropdown 2 (`Update Priority Rule`):**
    - `Critical Security Patch First (Urgency)`
    - `Most Overlapping Towers First (Degree)`
    - `Highest User Traffic First (Load)`
  - **Buttons:**
    - `🎲 Randomize Towers Needing Update` (Randomly sets ~75% of Pune towers to `needs_update = 1` and ~25% to `0`, redraws map immediately).
    - `▶ Run Coloring & Schedule` (Green `success` button — runs selected C++ engine(s), prints to terminal, colors map pins, updates UI)[cite: 25].
    - `↻ Reset Map` (Red `danger` button — clears colors back to unscheduled state)[cite: 25].
  - **Green Summary Text Block:**
    - Displays `Active Towers`, `Total Update Batches (Colors C)`, `Target Per Batch (γ)`, `Greedy-FF Skewed RSD%`, and `VFF Balanced RSD%`[cite: 3, 4, 25].
  - **Bottom Comparison Box (`ScrolledText`):**
    - Shows the side-by-side table of `OpenMP` vs `MPI` vs `Hybrid` (Execution Time in ms, Initial RSD%, Final RSD%, Conflicts Resolved, and Batch-by-Batch Tower Counts before vs. after balancing)[cite: 4, 25].

- [x] **Task 3.3: Right Panel Dark Radar Map Canvas**
  - Draw simple background road/river lines and Pune area labels (`HINJEWADI`, `KOTHRUD`, `SHIVAJINAGAR`, `DECCAN`, `CAMP`, `VIMAN NAGAR`, `HADAPSAR`, `SWARGATE`, `KATRAJ`, `PIMPRI`)[cite: 24].
  - Draw interference edges connecting active overlapping towers.
  - Draw circular tower pins with carrier labels (`JIO`, `AIRTEL`, `VI`, `BSNL`)[cite: 24]:
    - **Grey (`#4b5563`):** No Update Needed (Skipped).
    - **Yellow (`#f59e0b`):** Needs Update (Waiting to be colored).
    - **Bright Batch Colors after coloring:** Pink (`#ec4899`), Blue (`#3b82f6`), Red (`#ef4444`), Green (`#10b981`), Purple (`#a855f7`), Cyan (`#06b6d4`)[cite: 24].
  - Add hover tooltip or click popup showing Tower details (Area, Carrier, Users, Priority, Assigned Batch).

---

### Phase 4: Simple Compile & Run Script (`run.sh`)
- [x] **Task 4.1:** Create `run.sh` (and automatic compilation check inside `newui.py`) that compiles the 3 C++ files using:
  - `g++ -fopenmp engine_omp.cpp -o engine_omp`
  - `mpicxx engine_mpi.cpp -o engine_mpi`
  - `mpicxx -fopenmp engine_hybrid.cpp -o engine_hybrid`
  - And launches `python3 newui.py`.