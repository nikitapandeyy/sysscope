# Devlog Index

Engineering log for **Mini Process Monitor (`minimon`)**. Every task is verified, documented, and committed individually.

| Task # | Title | Date | Commit Hash | Status |
|:-------|:------|:-----|:------------|:-------|
| [T1.1](001-repo-bootstrap.md) | Repo bootstrap: folders, CMake, and devlog | 2026-11-23 | `db9f7b3` | Completed |
| [T1.2](002-proc-basics.md) | `/proc` basics and telemetry documentation | 2026-11-23 | `4c0fe11` | Completed |
| [T1.3](003-stat-experiment.md) | `/proc/self/stat` parsing experiment | 2026-11-23 | `5f1fde1` | Completed |
| [T2.1](004-test-framework.md) | Core library, Process struct, and GoogleTest setup | 2026-11-23 | `8567203` | Completed |
| [T2.2](005-pid-enumeration.md) | PID enumeration via proc root directory scanning | 2026-11-23 | `f3bf2cb` | Completed |
| [T2.3](006-stat-parser.md) | Robust `/proc/[pid]/stat` parser with comm handling | 2026-11-23 | `3049712` | Completed |
| [T2.4](007-status-cmdline-parser.md) | `/proc/[pid]/status` & `cmdline` parsers | 2026-11-23 | `6c35c42` | Completed |
