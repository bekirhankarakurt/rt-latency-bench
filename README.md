# rt-latency-bench

Measure Linux wake-up latency like `cyclictest` — with SCHED_FIFO/RR, CPU pinning, background load, percentile histograms and CSV/JSON export — in dependency-free modern C++17.

[![CI](https://github.com/bekirhankarakurt/rt-latency-bench/actions/workflows/ci.yml/badge.svg)](https://github.com/bekirhankarakurt/rt-latency-bench/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C.svg)](https://en.cppreference.com/w/cpp/17)
[![Platform: Linux](https://img.shields.io/badge/platform-Linux-informational.svg)](https://github.com/bekirhankarakurt/rt-latency-bench#requirements)

## Why / What

- **Answers one question:** how late does a periodic real-time thread wake up, and how does that change with scheduling policy, pinning and system load?
- **Accurate timing loop:** `clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME)` against a fixed schedule, so jitter never accumulates; missed periods are counted as overruns.
- **Allocation-free hot path:** fixed-geometry histograms (min/avg/max/p50/p99/p99.9) with microsecond resolution; no malloc inside the measurement loop.
- **Runs without privileges:** if `SCHED_FIFO`, `mlockall` or pinning is refused, the run continues as `SCHED_OTHER` and says so in the report.
- **Runs without hardware:** `--simulate` (deterministic synthetic latencies) and `--replay` (recorded latencies) exercise the whole pipeline on any machine, including CI.

## Demo

Real output from `rtlb -t 2 -a 0-1 -l cpu,mem -d 5 --histogram` (SCHED_FIFO/80, two pinned threads, CPU + memory load, 4-core VM):

```text
rt-latency-bench 1.0.0 | mode=live | policy=fifo/80 | interval=1000 us | duration=5.0 s | load=cpu,mem

thread   cpu  policy       samples     min_us     avg_us     max_us     p50_us     p99_us   p99.9_us  overruns
     0     0  fifo/80         5000       26.1       57.6     6748.9       47.0      148.0     1402.0        14
     1     1  fifo/80         5000       24.8       63.6     2699.0       54.0      181.0     1578.0        10
   all     -  fifo/80        10000       24.8       60.6     6748.9       50.0      163.0     1578.0        24

latency histogram (bar length is log-scaled)
  20 - 50        us       5127  ########################################
  50 - 100       us       4331  #######################################
  100 - 200      us        472  #############################
  200 - 500      us         37  #################
  500 - 1000     us         18  ##############
  1000 - 2000    us         12  ############
  2000 - 5000    us          2  #####
  5000 - 10000   us          1  ###
```

Comparison mode (`rtlb compare -a 0 -l cpu -d 5`): the same workload under SCHED_FIFO (A) and SCHED_OTHER (B):

```text
metric          A (us)      B (us)       B - A     B / A
min               23.4        37.6        14.2     1.61x
avg               71.1       112.8        41.6     1.59x
p50               60.0        96.0        36.0     1.60x
p99              203.0       313.0       110.0     1.54x
p99.9           1434.0      3748.0      2314.0     2.61x
max             2528.8      3792.1      1263.3     1.50x
overruns            15          37
```

Numbers come from a shared virtual machine and are illustrative; run it on your own target.

## Quick start (60 seconds)

```bash
git clone https://github.com/bekirhankarakurt/rt-latency-bench.git
cd rt-latency-bench
cmake -S . -B build && cmake --build build -j
ctest --test-dir build --output-on-failure
./build/rtlb --simulate --histogram          # no privileges needed
sudo ./build/rtlb -t 2 -a 0-1 --mlock -d 10  # real SCHED_FIFO measurement
```

## Requirements

| Item | Requirement |
| --- | --- |
| OS | Linux (uses `clock_nanosleep`, `sched_setscheduler`, `sched_setaffinity`, `mlockall`) |
| Compiler | C++17: GCC or Clang (CI: GCC 13, Clang 18) |
| Build system | CMake 3.16+ |
| Dependencies | None (standard library and POSIX only) |
| Privileges | Optional. `CAP_SYS_NICE` for SCHED_FIFO/RR, `CAP_IPC_LOCK` for `--mlock`; without them the tool falls back and warns |

### Build options

| CMake option | Default | Effect |
| --- | --- | --- |
| `CMAKE_BUILD_TYPE` | `Release` | Standard CMake build type |
| `RTLB_BUILD_TESTS` | `ON` | Build the unit tests (run with `ctest`) |
| `RTLB_SANITIZE` | `OFF` | Build with AddressSanitizer + UndefinedBehaviorSanitizer |
| `RTLB_WERROR` | `OFF` | Treat warnings as errors (`-Wall -Wextra -Wpedantic -Wshadow -Wconversion` are always on) |

## Usage

```text
rtlb [run] [options]       measure wake-up latency
rtlb compare [options]     run twice (--policy vs --vs) and print the difference
```

| Option | Default | Description |
| --- | --- | --- |
| `-t, --threads N` | 1 | Measurement threads |
| `-i, --interval US` | 1000 | Period in microseconds |
| `-d, --duration S` | 10 | Run time in seconds (Ctrl-C stops early and still reports) |
| `-p, --policy P` | fifo | `fifo`, `rr` or `other` |
| `-P, --priority N` | 80 | Real-time priority 1-99 |
| `-a, --affinity LIST` | none | CPUs such as `0,2-3`; thread *i* uses entry *i* mod N |
| `-m, --mlock` | off | `mlockall(MCL_CURRENT\|MCL_FUTURE)` |
| `-l, --load LIST` | none | `cpu`, `mem`, `io` (comma-separated) or `none` |
| `--load-threads N` | 1 | Load threads per kind |
| `--mem-mib N` | 64 | Memory-load buffer per thread (MiB) |
| `--io-dir DIR` | temp dir | Where the IO load writes its scratch file |
| `--bucket-us N` / `--max-us N` | 1 / 10000 | Histogram bucket width and range |
| `--histogram` | off | Print the ASCII histogram |
| `--csv FILE` / `--json FILE` | none | Export results |
| `--threshold-us N` | none | Exit with status 3 if max latency exceeds N |
| `--simulate` / `--seed N` | off / 1 | Synthetic latencies in virtual time |
| `--replay FILE` | none | Replay recorded latencies (µs, one per line, `#` comments) |
| `--vs P` | other | Policy of the second run in `compare` |

Exit status: `0` ok, `1` runtime error, `2` usage error, `3` threshold exceeded.

### Recipes

```bash
# 1. Baseline of an isolated core (boot with isolcpus=3 for best results)
sudo ./build/rtlb -a 3 -P 90 --mlock -d 60 --histogram

# 2. Latency under full system stress, four pinned threads
sudo ./build/rtlb -t 4 -a 0-3 -l cpu,mem,io --load-threads 2 -d 120 --json stress.json

# 3. Quantify what real-time scheduling buys you
sudo ./build/rtlb compare -a 2 -l cpu,io -d 30

# 4. Regression gate for a CI or nightly job (fails the job above 200 us)
sudo ./build/rtlb -a 1 --mlock -d 300 --threshold-us 200 --csv nightly.csv

# 5. Analyse a recording or test tooling without hardware
./build/rtlb --replay examples/latencies.txt --histogram
./build/rtlb --simulate -t 4 -l cpu --seed 7 --json sim.json
```

## Architecture

```text
 CLI (cli.cpp) ──► Config ──► run() (runner.cpp)
                                 │
          ┌──────────────────────┼──────────────────────────┐
     Live mode              Simulate mode              Replay mode
  worker thread / CPU     simulator.cpp              replay.cpp
  platform.cpp:           (seeded PRNG,              (µs file,
  pin, SCHED_*,           virtual time)              round-robin
  clock_nanosleep ◄─ LoadSet (load.cpp:                per thread)
  TIMER_ABSTIME       cpu / mem / io threads)
          └───────────────► Histogram (per thread) ◄────────┘
                                 │ merge
                         Report ─► report.cpp ─► table | ASCII histogram | CSV | JSON | compare
```

Design trade-offs:

- **Histogram instead of storing every sample:** constant memory and no allocation in the loop, at the cost of bucket-width resolution (1 µs by default). Percentiles report the upper edge of the bucket and never exceed the observed maximum; min, avg and max are exact.
- **Absolute-time sleeping:** each deadline is `start + n * interval`, so scheduling noise does not drift the schedule. A period that is missed entirely is skipped and counted as an overrun instead of bursting to catch up.
- **Graceful degradation over hard failure:** unprivileged runs still produce data; every downgrade is recorded as a warning in the table and JSON, and the *effective* policy is read back with `sched_getscheduler`.
- **`mlockall` after thread creation:** a low `RLIMIT_MEMLOCK` would otherwise make thread creation fail under `MCL_FUTURE`.
- **Simulation is a model, not a measurement:** it exists to test the tooling and demonstrate comparisons; never quote it as hardware data.

## Data formats

CSV (`--csv`), one row per thread plus `all`; times in microseconds, `cpu` is `-1` when not pinned:

| Column | Meaning |
| --- | --- |
| `thread`, `cpu`, `policy` | Thread id, pinned CPU, effective policy/priority (e.g. `fifo/80`) |
| `samples` | Number of wake-ups measured |
| `min_us`, `avg_us`, `max_us` | Exact extremes and mean |
| `p50_us`, `p99_us`, `p999_us` | Percentiles from the histogram |
| `overruns` | Periods missed entirely |

JSON (`--json`) top-level keys:

| Key | Content |
| --- | --- |
| `tool`, `version`, `mode` | Producer, version, `live` / `simulate` / `replay` |
| `config` | Threads, interval, duration, requested policy/priority, loads, histogram geometry, seed |
| `warnings` | Fallbacks that happened (policy, pinning, mlock) |
| `threads[]`, `total` | Same statistics as the CSV; `threads[]` also has `id`, `cpu`, `policy`, `priority` |
| `histogram` | `bucket_us`, `overflow` count, non-empty `bins` as `[lower_edge_us, count]` |

Replay input: plain text, one latency in microseconds per line (`12.5`); blank lines and `#` comments are ignored.

## Testing

```bash
ctest --test-dir build --output-on-failure                    # 7 suites, no external framework
cmake -S . -B build-asan -DRTLB_SANITIZE=ON -DRTLB_WERROR=ON && cmake --build build-asan -j && ctest --test-dir build-asan
```

Suites cover the histogram maths, CLI parsing, platform wrappers, simulator and replay, load generators (including cleanup of the IO scratch file), the runner (live runs use `SCHED_OTHER` or tolerate fallback, so they pass unprivileged) and every output format. CI builds with GCC and Clang with `-Werror`, plus an ASan/UBSan job.

## Troubleshooting / FAQ

**`warning: ... sched_setscheduler(fifo, 80): Operation not permitted`**
Run with `sudo`, grant `CAP_SYS_NICE` (`sudo setcap cap_sys_nice,cap_ipc_lock+ep build/rtlb`), or raise `ulimit -r`. The results are still valid, but for `SCHED_OTHER`.

**`warning: memory not locked: mlockall: Cannot allocate memory`**
`RLIMIT_MEMLOCK` is too small; use `ulimit -l unlimited` or run privileged.

**Latencies of several milliseconds with FIFO**
Typical for VMs, non-isolated cores, SMIs and power management. Pin to an isolated core (`isolcpus=`, `nohz_full=`), disable deep C-states, and use a `PREEMPT_RT` kernel for hard guarantees.

**The system freezes at priority 99 with CPU load**
Spinning real-time threads can starve kernel threads; keep the priority below IRQ threads (about 50 on RT kernels) and keep `kernel.sched_rt_runtime_us` at its default throttling.

**Can I trust `--simulate`?** It is a deterministic model for tooling and CI; only `live` mode measures your system.

## Project layout

```text
.
├── CMakeLists.txt          build, options, test registration
├── apps/main.cpp           rtlb entry point, signal handling, exit codes
├── include/rtlb/           public headers (one per module)
├── src/
│   ├── histogram.cpp       fixed-geometry histogram, percentiles, summaries
│   ├── config.cpp          configuration, parsing helpers, validation
│   ├── cli.cpp             argument parser and --help text
│   ├── platform.cpp        clock_nanosleep, affinity, SCHED_*, mlockall
│   ├── load.cpp            CPU / memory / IO load generators (RAII)
│   ├── simulator.cpp       synthetic latency model
│   ├── replay.cpp          recorded-latency loader
│   ├── runner.cpp          live / simulated / replay orchestration
│   └── report.cpp          table, ASCII histogram, CSV, JSON, comparison
├── tests/                  dependency-free unit tests (test.hpp harness)
├── examples/latencies.txt  sample recording for --replay
└── .github/workflows/      CI: GCC, Clang, ASan + UBSan
```

## Roadmap

- [x] Periodic threads with absolute-time sleeping, SCHED_FIFO/RR/OTHER, affinity, mlockall
- [x] CPU, memory and IO load generators
- [x] Histogram with min/avg/max/p50/p99/p99.9, ASCII view, CSV and JSON export
- [x] Policy comparison mode, simulation and replay modes, CI with sanitizers
- [ ] Compare two exported JSON files offline
- [ ] Per-thread histograms in the JSON export
- [ ] Optional ftrace marker on worst-case latency

## License

MIT — see [LICENSE](LICENSE).

Bekirhan Karakurt - [github.com/bekirhankarakurt](https://github.com/bekirhankarakurt)
