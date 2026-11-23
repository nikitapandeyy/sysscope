# Devlog Task 017: JSON Export Mode and Documentation

## Task
Implement the `--json` structured export mode in `minimon`, serializing system overview telemetry and process status into valid JSON with RFC 8259-compliant string escaping, and document the export schema in `docs/learning/json-format.md`.

---

## What I Did
1. **JSON String Escaping**: Implemented `minimon::json_escape(std::string_view)` in [src/display.cpp](file:///Users/nikitapandey/Downloads/SysScope/src/display.cpp) and [src/display.hpp](file:///Users/nikitapandey/Downloads/SysScope/src/display.hpp). Handles quotes (`\"`), backslashes (`\\`), ASCII control characters (`\b`, `\f`, `\n`, `\r`, `\t`), and generic control bytes `< 0x20` formatted as `\u00xx`.
2. **JSON Serializer**: Implemented `minimon::render_json(const std::vector<Process>&, const SystemSummary&, size_t)` emitting structured JSON containing schema version `"1.0"`, `system` object (tasks breakdown, memory in kB and percentage, 1/5/15 minute load averages), and `processes` array of process records.
3. **Execution Routing**: Updated `minimon::run_single_shot()` and `main()` in [src/main.cpp](file:///Users/nikitapandey/Downloads/SysScope/src/main.cpp) so passing `--json` executes in single-shot mode without emitting ANSI terminal escape sequences that would corrupt automated parsers.
4. **Schema Documentation**: Authored [docs/learning/json-format.md](file:///Users/nikitapandey/Downloads/SysScope/docs/learning/json-format.md) documenting top-level keys, data types, field sources from `/proc`, and integration examples with `jq`.
5. **Testing**: Added unit tests in [tests/test_display.cpp](file:///Users/nikitapandey/Downloads/SysScope/tests/test_display.cpp) covering `JsonEscape`, `RenderJsonOutput`, and fixture-driven `RunSingleShotJsonOnFixture`.

---

## Why I Did It This Way
- **Zero External Dependencies**: Implemented direct stream-based JSON generation without third-party libraries (e.g. nlohmann/json). This keeps `minimon` lightweight, easy to understand, and dependency-free while strictly adhering to JSON specification rules.
- **Explicit Schema Versioning**: Added `"schema_version": "1.0"` to the root JSON object so downstream consumers can inspect version compatibility before ingestion.
- **Single-Shot Coercion**: If `--json` is supplied, `main()` directs execution to `run_single_shot` rather than `run_live_loop`. TUI control sequences (such as clearing the screen or moving the cursor) are not valid JSON tokens and would break utilities like `jq` or HTTP scrapers.

---

## Problems Encountered
- **Problem**: When processes have command lines containing quotation marks, backslashes, or tabs (e.g. `python3 -c "print('hi')"`), naive string printing produces invalid JSON syntax.
  - **Symptom**: `python3 -m json.tool` fails with `JSONDecodeError: Expecting ',' delimiter`.
  - **Root Cause**: JSON string literals require escaping for `"` and `\` and disallowed control characters.
  - **Solution**: Developed `json_escape()` which iterates through characters and substitutes required escape sequences.

---

## How I Solved Them
- Validated JSON output across edge cases using unit tests in GoogleTest and piped CLI fixture output into `python3 -m json.tool` to confirm compliance with RFC 8259.

---

## Linux/OS Concepts Involved
- **Inter-Process Data Ingestion**: Machine-parseable interfaces (JSON, CSV, Protobuf) bridge kernel telemetry to distributed monitoring frameworks (Prometheus node-exporter, Datadog agents, custom daemon scrapers).
- **Control Character Sanitization**: `/proc/[pid]/cmdline` and process names may contain arbitrary byte sequences provided by userspace during `execve`. Sanitization prevents injection vulnerabilities when serializing telemetry.

---

## How to Verify
```bash
# 1. Build and run all tests
cmake --build build && ctest --test-dir build --output-on-failure

# 2. Test JSON export against fixture
./build/minimon --json --proc-root tests/fixtures/proc_full

# 3. Verify RFC 8259 compliance via python json parser
./build/minimon --json --proc-root tests/fixtures/proc_full | python3 -m json.tool
```

---

## Open Questions / Follow-ups
- Phase 3 is now complete. Next milestone is Phase 4: Quality & Polish (integration test comparing metrics against real child processes, sanitizers, GDB transcript, GitHub Actions CI, and release tagging).
