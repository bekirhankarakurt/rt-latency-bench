# Security policy

## Supported versions

| Version | Supported |
| --- | --- |
| 1.0.x | Yes |
| < 1.0 | No |

## Reporting a vulnerability

Please do not open a public issue. Report privately through a
[GitHub Security Advisory](https://github.com/bekirhankarakurt/rt-latency-bench/security/advisories/new)
with a description, affected version and reproduction steps. You can expect an
acknowledgement within a few days and a fix or mitigation plan after triage.

`rtlb` can run privileged (`CAP_SYS_NICE`, `CAP_IPC_LOCK`) and writes files you
name with `--csv`, `--json` and `--io-dir`; reports about unsafe handling of
these paths or of replay input are in scope.
