# Changelog

All notable changes to this project are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project adheres to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-11-04

### Added
- Periodic measurement threads using `clock_nanosleep` with `TIMER_ABSTIME`, overrun accounting and Ctrl-C handling.
- `SCHED_FIFO`, `SCHED_RR` and `SCHED_OTHER` scheduling, CPU affinity and `mlockall`, with graceful fallback and warnings when privileges are missing.
- CPU, memory and IO load generators.
- Allocation-free latency histograms with min/avg/max/p50/p99/p99.9, ASCII histogram, CSV and JSON export.
- `compare` command to contrast two scheduling policies under the same load.
- Deterministic `--simulate` mode and `--replay` of recorded latencies, so everything runs without hardware.
- `--threshold-us` exit-code gate for CI use.
- Unit tests, GCC/Clang/ASan/UBSan CI, clang-tidy, cppcheck, CodeQL, coverage, API docs and release packaging.

[1.0.0]: https://github.com/bekirhankarakurt/rt-latency-bench/releases/tag/v1.0.0
