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
| [T2.4](007-status-cmdline-parser.md) | `/proc/[pid]/status` & `cmdline` parsers | 2026-11-23 | `67ecf7c` | Completed |
| [T2.5](008-username-lookup.md) | Username lookup from UID with caching and fallback | 2026-11-23 | `e1ab660` | Completed |
| [T2.6](009-system-parsers.md) | System memory (`/proc/meminfo`) & CPU (`/proc/stat`) parsers | 2026-11-23 | `e4c4bfb` | Completed |
| [T2.7](010-cpu-tracker.md) | Two-sample CPU tracker & PID reuse detection | 2026-11-23 | `a31f413` | Completed |
| [T2.8](011-read-all-processes.md) | Unified process listing & vanished process handling | 2026-11-23 | `78dc8c1` | Completed |
| [T3.1](012-filter-module.md) | Search & filter module with multi-criteria support | 2026-11-23 | `e998334` | Completed |
| [T3.2](013-sort-module.md) | Sorting module by CPU, memory, PID, and name | 2026-11-23 | `cc3eb88` | Completed |
| [T3.3](014-table-renderer.md) | Terminal table renderer & system summary banner | 2026-11-23 | `aac4a58` | Completed |
| [T3.4](015-cli-parser.md) | CLI argument parsing & help documentation | 2026-11-23 | `e411178` | Completed |
| [T3.5](016-live-refresh-mode.md) | Live refresh mode & signal handling | 2026-11-23 | `c1c8342` | Completed |
| [T3.6](017-json-export.md) | JSON export mode and format documentation | 2026-11-23 | `11917b4` | Completed |
