# DVFS Controller

Lightweight DVFS controller using LIKWID performance counters.

## Requirements

- LIKWID library and headers in your environment.
- Access to performance counters (root or LIKWID access daemon).
- `cpupower` available to set frequencies.

## Build

```bash
make
```

## Run

Defaults are compiled in via `MONITOR_CPU_ID` and `CONTROLLER_CPU_ID` in
`dvfs/main.c`.

```bash
./dvfs_controller
```

Override at runtime:

```bash
./dvfs_controller -m 0 -c 1
```

- `-m`: CPU to monitor for performance counters.
- `-c`: CPU to run the controller on.

## Permissions

Typical options:

- Run as root.
- Or start the LIKWID access daemon and use it to access counters.

If `cpupower` fails, check your CPU governor and permissions.

## How it works

- The controller pins itself to `-c` (or `CONTROLLER_CPU_ID`).
- LIKWID perfmon is initialized over all CPUs.
- Counters are read from `-m` (or `MONITOR_CPU_ID`).
- Derived metrics drive the DVFS policy in `dvfs/dvfs_policy.c`.

## Config

Frequency levels are defined in `dvfs_settings.conf`.

Example:

```
LOW_FREQ_KHZ=800000
MED_FREQ_KHZ=1800000
HIGH_FREQ_KHZ=2500000
```

## Notes

- The controller is single-threaded; LIKWID CPU lists only define which CPUs
  are available for measurement.
- For minimal interference, pin the controller to a dedicated CPU.

## Example CPU layout

Example with two NUMA nodes (0-30 even on node0, 1-31 odd on node1):

```bash
# Monitor core 0, run controller on set-aside core 31
./dvfs_controller -m 0 -c 31
```

## Troubleshooting

- `ERROR: ThreadID greater than defined threads`:
  Use `./dvfs_controller` without `likwid-pin`. The controller manages pinning
  itself. If you must use `likwid-pin`, include the monitor CPU in the pin list.
- `Failed to add event set`:
  Verify event names for your CPU with `likwid-perfctr -e`.
- `Failed to set frequency`:
  Ensure `cpupower` works on your platform and the governor permits manual
  frequency changes.
