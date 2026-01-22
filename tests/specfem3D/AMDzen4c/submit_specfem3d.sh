#!/bin/bash
#SBATCH -J specfem3D
#SBATCH -N 1
#SBATCH --partition=zen4
#SBATCH --reservation=cpufreq
#SBATCH --nodelist=rpc-97-[1-20]
#SBATCH --ntasks-per-node=256
#SBATCH --cpus-per-task=1
#SBATCH --exclusive
#SBATCH --time=4:00:00
#SBATCH -o specfem3D/specfem3D.%A.out
#SBATCH -e specfem3D/specfem3D.%A.err

# Run run_this_example.sh to generate the necessary input files before executing this script.
# time step parameters
# NSTEP                           = 1000000
# DT                              = 0.05

set -euo pipefail

SPECFEM3D_DIR="/mnt/SHARED-AREA/HPC-Benchmarks/specfem3d"
EXAMPLE_DIR="${SPECFEM3D_DIR}/EXAMPLES/applications/homogeneous_halfspace"

RESULT_DIR="${SLURM_SUBMIT_DIR}/profiles"
mkdir -p "${RESULT_DIR}"

module purge
module load openmpi/4.1.8 likwid/5.4.1-daemon

echo "Launching specfem3D with 256 MPI ranks"

cd "${EXAMPLE_DIR}"

likwid-perfctr -f -c 0,128 -g ENERGY -t 500ms -O \
  -- srun --mpi=pmix --cpu-bind=cores --distribution=block:block ./bin/xspecfem3D \
  2> "${RESULT_DIR}/specfem3D.${SLURM_JOB_ID}.prof"