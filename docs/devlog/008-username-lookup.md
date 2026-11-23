# Devlog T2.5: Username Lookup from UID

## Task
Implement username resolution from numeric UIDs using POSIX `getpwuid_r`, an in-memory caching layer to avoid repetitive system database lookups, and a safe numeric fallback when users are unmapped.

## What I did
- Added `ProcReader::get_username(uid_t uid)` and `ProcReader::clear_user_cache()` in `src/proc_reader.hpp` and `src/proc_reader.cpp`.
- Implemented cached lookups backed by `mutable std::unordered_map<uid_t, std::string> user_cache_`:
  - Queries `getpwuid_r` with a local 1024-byte stack buffer to safely retrieve `passwd->pw_name`.
  - If the user database query succeeds, stores and returns the username (e.g. UID 0 -> `"root"`).
  - If the UID is not found (e.g. deleted user account or unmapped container UID), falls back to the string representation of the numeric UID (e.g. `"9999999"`).
  - Caches results to guarantee that subsequent queries for the same UID incur zero disk or NSS lookups.
- Integrated username lookup directly into `ProcReader::read_status()`, automatically populating `out.user` from `out.uid`.
- Authored unit tests in `tests/test_proc_reader.cpp` verifying root lookup, numeric fallbacks, caching idempotency, and status reader integration.

## Why I did it this way
- System calls like `getpwuid` consult `/etc/passwd` or network directory services (NIS, LDAP, SSSD). In a monitor scanning 500+ processes per interval, uncached lookups would repeatedly hit disk/network services, causing high CPU overhead and monitoring jitter.
- `getpwuid_r` is the reentrant POSIX standard, eliminating race conditions associated with `getpwuid`'s static internal return buffer.
- Alternatives considered: Calling `getpwuid` without caching. Rejected due to the severe performance degradation when polling hundreds of processes every 1-2 seconds.

## Problems encountered
None. All 19 tests passed cleanly on the first run.

## How I solved them
N/A (no problems encountered).

## Linux concepts involved
- POSIX user identity: User IDs (`uid_t`) and `/etc/passwd` account mapping via `struct passwd`.
- Reentrant system interfaces: `getpwuid_r(3)` providing thread-safe password database inspection.
- Unmapped UIDs: daemons running under dynamic or isolated UIDs that lack a name in `/etc/passwd`.

## How to verify
```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

## Open questions
- None. Next task is T2.6: system memory parser (`/proc/meminfo`) and total CPU time (`/proc/stat`).
