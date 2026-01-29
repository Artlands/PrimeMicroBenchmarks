#!/bin/bash
#SBATCH -J spechpc_weather
#SBATCH -N 1
#SBATCH --reservation=cpufreq
#SBATCH --nodelist=rpc-97-[16-20]
#SBATCH --ntasks-per-node=256
#SBATCH --cpus-per-task=1
#SBATCH --exclusive
#SBATCH --time=4:00:00
#SBATCH -o 635.weather_s/spechpc_weather.%A.out
#SBATCH -e 635.weather_s/spechpc_weather.%A.err

set -euo pipefail

SPECHPC_DIR="/mnt/SHARED-AREA/HPC-Benchmarks/SPEC/SPEC-HPC/SPEChpc"
RUN_SUBDIR="run/run_base_ref_gnu_mpi.0000"

RESULT_DIR="${SLURM_SUBMIT_DIR}/profiles"
mkdir -p "${RESULT_DIR}"

module purge
module load mpich/4.3.2 likwid/5.4.1-daemon

cd "${SPECHPC_DIR}"
source shrc
go "635.weather_s" "${RUN_SUBDIR}"

echo "Launching 635.weather with 256 MPI ranks"

likwid-perfctr -f -c 0,128 -g ENERGY -t 500ms -O \
  -- srun --mpi=pmix --cpu-bind=cores --distribution=block:block ./weather output6.ref.txt 192000 10000 24000 1250 600 100 6 \
  2> "${RESULT_DIR}/spechpc_weather.${SLURM_JOB_ID}.prof"