#!/usr/bin/env bash
# ============================================================================
# 5G Cell Tower Update Scheduler (Balanced Graph Coloring)
# Compilation and Launcher Script
# ============================================================================

set -e

echo "============================================================"
echo "  5G TOWER UPDATE SCHEDULER - COMPILATION & RUN SCRIPT      "
echo "============================================================"

# Detect OS and set appropriate OpenMP compiler flags
OS="$(uname -s)"
echo "[Info] Detected Operating System: $OS"

if [ "$OS" = "Darwin" ]; then
    # macOS: Homebrew libomp support
    LIBOMP_PREFIX=""
    if [ -d "/opt/homebrew/opt/libomp" ]; then
        LIBOMP_PREFIX="/opt/homebrew/opt/libomp"
    elif [ -d "/usr/local/opt/libomp" ]; then
        LIBOMP_PREFIX="/usr/local/opt/libomp"
    fi

    if [ -n "$LIBOMP_PREFIX" ]; then
        echo "[Info] Using Homebrew libomp at: $LIBOMP_PREFIX"
        OMP_FLAGS="-Xpreprocessor -fopenmp -I${LIBOMP_PREFIX}/include -L${LIBOMP_PREFIX}/lib -lomp"
    else
        OMP_FLAGS="-fopenmp"
    fi
    CXX_COMPILER="clang++"
else
    # Linux / other Unix
    OMP_FLAGS="-fopenmp"
    CXX_COMPILER="g++"
fi

# 1. Compile Engine 1: Pure OpenMP
echo "[1/3] Compiling engine_omp.cpp ..."
$CXX_COMPILER $OMP_FLAGS engine_omp.cpp -o engine_omp
echo "      -> Successfully compiled engine_omp"

# 2. Compile Engine 2: Pure MPI
echo "[2/3] Compiling engine_mpi.cpp ..."
mpicxx engine_mpi.cpp -o engine_mpi
echo "      -> Successfully compiled engine_mpi"

# 3. Compile Engine 3: Hybrid OpenMP + MPI
echo "[3/3] Compiling engine_hybrid.cpp ..."
mpicxx $OMP_FLAGS engine_hybrid.cpp -o engine_hybrid
echo "      -> Successfully compiled engine_hybrid"

echo "============================================================"
echo "  All 3 Parallel Engines Compiled Successfully!            "
echo "  Launching GUI (newui.py)...                              "
echo "============================================================"

# Launch the ttkbootstrap GUI
python3 newui.py

