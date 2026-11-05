# 📡 5G Cell Tower Update Scheduler — Balanced Graph Coloring

A parallel computing mini-project that uses **Balanced Graph Coloring** to schedule 5G cell tower firmware updates efficiently and safely.

**Course:** Parallel Programming Lab — Mini-Project

## 👥 Team Members

| Name | MIS |
|---|---|
| Shudhodhan Jogdand | 241080028 |
| Sairaj Desai | 241080011 |

**Reference Paper:** *Algorithms for Balanced Graph Colorings with Applications in Parallel Computing* — Lu et al., IEEE Transactions on Parallel and Distributed Systems, Vol. 28, No. 5, May 2017.

---

## 📌 1. Project Overview

Telecommunication operators manage large networks of 5G cell towers that require regular firmware updates, security patches, and maintenance.

Updating neighboring towers simultaneously can cause temporary coverage gaps and disrupt connectivity. Therefore, the update process must be scheduled such that:

- Neighboring towers are never updated simultaneously.
- Update batches are distributed as evenly as possible.
- Available CPU threads and processing resources are utilized efficiently.
- The total number of update batches remains as small as possible.

This project models the scheduling problem using **Distance-1 Graph Coloring** and improves the scheduling efficiency through **Balanced Graph Coloring**.

---

## 🌐 2. Real-World Motivation

### 5G Telecom Firmware Update Scheduling — Pune City Network

Consider a network of 5G cell towers operated by telecom providers such as Jio, Airtel, Vi, and BSNL across Pune.

During a midnight firmware update:

1. Each tower represents a vertex in a graph.
2. An edge connects two towers whose coverage areas overlap or whose simultaneous rebooting could cause a service disruption.
3. Each color represents an independent update batch.
4. Towers assigned the same color can be updated concurrently, provided no two adjacent towers share that color.

### The Problem with Traditional Greedy Coloring

Traditional Greedy First-Fit (`Greedy-FF`) assigns each vertex the first available valid color.

Although this approach produces a valid coloring and often uses relatively few colors, it can create highly imbalanced batches.

For example:

| Update Batch | Greedy-FF | Balanced Coloring |
|---|---:|---:|
| Batch 1 | 18 towers | 10 towers |
| Batch 2 | 12 towers | 10 towers |
| Batch 3 | 6 towers | 10 towers |
| Batch 4 | 4 towers | 10 towers |
| **Total** | **40 towers** | **40 towers** |

An imbalanced schedule can overload update servers while leaving other processing resources underutilized.

### Our Solution: Vertex-Centric First-Fit (VFF)

We implement a guided balancing strategy consisting of two major stages.

**Stage 1 — Priority-Ordered Initial Coloring (`Greedy-FF`)**

- Towers are processed according to a selected priority.
- Each tower is assigned the first permissible color.
- The initial number of batches, \(C\), is determined.
- The target average batch size is calculated.

\[
\gamma = \frac{|V|}{C}
\]

**Stage 2 — Parallel Vertex-Centric First-Fit (`VFF`) Balancing**

- Identify over-full and under-full batches.
- Select candidate towers from over-full batches.
- Attempt to migrate them into permissible under-full batches.
- Check adjacency constraints before accepting a migration.
- Use parallel workers to evaluate candidate moves.
- Repeat balancing iterations until no further valid improvements are found or a stopping condition is reached.

The objective is to improve batch balance **without increasing the number of colors**.

---

## ⚙️ 3. Parallel Computing Engines

The application supports three parallel programming paradigms that can be executed and benchmarked through the graphical interface.

### 1. OpenMP Engine

**File:** `engine_omp.cpp`

Implements shared-memory parallelism using OpenMP.

Features:
- Parallel vertex processing using `#pragma omp parallel for`.
- Shared graph and coloring data.
- Synchronization using atomic operations, critical sections, or other appropriate mechanisms.
- Multi-threaded execution on CPU cores.

### 2. MPI Engine

**File:** `engine_mpi.cpp`

Implements distributed-memory parallelism using MPI.

Features:
- Multiple processes using MPI ranks.
- Graph vertex partitioning across processes.
- Communication between ranks.
- Global synchronization using `MPI_Allreduce`.
- Distributed processing of balancing operations.

### 3. Hybrid MPI + OpenMP Engine

**File:** `engine_hybrid.cpp`

Combines distributed-memory and shared-memory parallelism.

Features:
- MPI distributes work across processes.
- OpenMP parallelizes work within each process.
- Combines inter-process communication with intra-process multithreading.
- Enables experimentation with hybrid parallel execution.

---

## 📊 4. Project Screenshots

The following section is reserved for screenshots of the application, its scheduling results, and performance comparisons.

### Main Application Interface

<!-- Replace the placeholder below with your main GUI screenshot -->

![Main Application Interface](screenshots/main_interface.png)

### 5G Tower Network Visualization

<!-- Replace the placeholder below with your tower network screenshot -->

![5G Tower Network Visualization](screenshots/tower_network.png)

### Balanced Graph Coloring Results

<!-- Replace the placeholder below with your coloring and batch distribution screenshot -->

![Balanced Graph Coloring Results](screenshots/balanced_coloring.png)

### Parallel Engine Benchmark

<!-- Replace the placeholder below with your performance comparison screenshot -->

![Parallel Engine Benchmark](screenshots/benchmark_results.png)

> **Note:** Create a `screenshots/` directory in your repository and place your screenshots inside it using the filenames shown above. You can rename the image paths to match your actual screenshots.

---

## 🗂️ 5. Project Structure

```text
5G-Cell-Tower-Update-Scheduler/
│
├── README.md
│
├── newui.py
│   └── Dark-themed ttkbootstrap GUI
│       └── Pune 5G tower network visualization
│
├── engine_omp.cpp
│   └── OpenMP parallel coloring engine
│
├── engine_mpi.cpp
│   └── MPI distributed coloring engine
│
├── engine_hybrid.cpp
│   └── Hybrid MPI + OpenMP coloring engine
│
├── setup_linux.sh
│   └── Linux dependency installation and build script
│
├── setup_mac.sh
│   └── macOS dependency installation and build script
│
├── setup_windows.bat
│   └── Windows dependency installation and build script
│
└── screenshots/
    ├── main_interface.png
    ├── tower_network.png
    ├── balanced_coloring.png
    └── benchmark_results.png
```

---

## 🛠️ 6. Technologies Used

| Technology | Purpose |
|---|---|
| C++ | Graph coloring algorithms and parallel engines |
| OpenMP | Shared-memory parallel processing |
| MPI | Distributed-memory parallel processing |
| Hybrid MPI + OpenMP | Combined parallel execution |
| Python | GUI and application orchestration |
| ttkbootstrap | Dark-themed graphical interface |
| GCC / G++ | C++ compilation |
| OpenMPI | MPI implementation |
| Homebrew | macOS dependency management |

---

## 🚀 7. Installation and Execution

The project includes setup scripts for Linux, macOS, and Windows.

### Linux — Ubuntu / Debian / Fedora

```bash
chmod +x setup_linux.sh
./setup_linux.sh
```

The script installs the required dependencies, compiles the available C++ engines, and launches the GUI.

### macOS

Requires [Homebrew](https://brew.sh/).

```bash
chmod +x setup_mac.sh
./setup_mac.sh
```

The script installs GCC, OpenMPI, OpenMP support, Python dependencies, and compiles the parallel engines before launching the application.

### Windows

Run the following file:

```text
setup_windows.bat
```

The setup requires Python, a compatible C++ compiler with OpenMP support, and an MPI implementation.

> **Platform note:** The Windows MPI build may require additional compiler and MPI configuration depending on whether MS-MPI, MSYS2, or another compatible MPI toolchain is used.

---

## 📈 8. Performance Evaluation

The project enables comparative analysis of three parallel computing paradigms.

The following metrics can be used to evaluate the implementations:

| Metric | Description |
|---|---|
| Execution Time | Time required to complete graph coloring and balancing |
| Number of Colors | Total number of update batches |
| Batch Distribution | Number of towers assigned to each batch |
| Speedup | Improvement in execution time compared with sequential execution |
| Parallel Efficiency | Effectiveness of utilizing available parallel resources |
| Load Balance | Distribution of work among batches or processing units |

### Speedup

\[
S(p)=\frac{T_1}{T_p}
\]

Where:

- \(T_1\) = sequential execution time
- \(T_p\) = parallel execution time using \(p\) processing units

### Parallel Efficiency

\[
E(p)=\frac{S(p)}{p}
\]

These metrics help compare the performance of OpenMP, MPI, and Hybrid MPI + OpenMP implementations.

---

## 🎯 9. Project Objectives

- Implement Distance-1 Graph Coloring for a practical telecom scheduling problem.
- Improve batch distribution using Balanced Graph Coloring.
- Implement parallel algorithms using OpenMP and MPI.
- Explore hybrid parallel programming.
- Compare execution time and parallel performance.
- Visualize tower networks, coloring assignments, and scheduling results through a GUI.

---

## 🔮 10. Future Scope

Potential improvements include:

- Integration with real-world telecom tower topology data.
- Dynamic scheduling based on real-time network traffic.
- Priority-based firmware update scheduling.
- Fault-tolerant scheduling for failed or unavailable towers.
- Advanced load-balancing heuristics.
- GPU-accelerated graph coloring.
- Real-time monitoring and visualization of update progress.

---

## 📚 11. References

1. Lu et al., *Algorithms for Balanced Graph Colorings with Applications in Parallel Computing*, IEEE Transactions on Parallel and Distributed Systems, Vol. 28, No. 5, May 2017.
2. OpenMP Architecture Review Board, [OpenMP API Specification](https://www.openmp.org/specifications/).
3. MPI Forum, [MPI Standard](https://www.mpi-forum.org/docs/).
4. [ttkbootstrap Documentation](https://ttkbootstrap.readthedocs.io/).

---

## 👨‍💻 Developed By

**Shudhodhan Jogdand** — MIS: 241080028  
**Sairaj Desai** — MIS: 241080011

Parallel Programming Lab Mini-Project
