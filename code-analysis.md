# OpenPLC Runtime Restructuring Notes

## Current Runtime Topology
- The PLC loop in `webserver/core/main.cpp:64` spins up the interactive server, initializes hardware, wires Modbus, and then calls protocol helpers (`initializeMB()`, `initializeSnap7()`, EtherCAT hooks) before entering the cyclic scan. This locates network bootstrapping alongside IEC runtime concerns.
- Protocol-facing threads are orchestrated inside `webserver/core/interactive_server.cpp:43`, where global flags (`run_modbus`, `run_dnp3`, `run_enip`, etc.) gate the per-protocol threads and expose start/stop commands to the Python GUI over a localhost socket.
- The socket dispatcher in `webserver/core/server.cpp:205` branches on `protocol_type` to invoke `processModbusMessage()` or `processEnipMessage()`, meaning decode logic lives side-by-side with connection handling and stream management.
- Modbus decoding (`webserver/core/modbus.cpp:1107`) is tightly coupled to the PLC buffers (`bool_input`, `int_output`, and friends from `ladder.h`). Each function code handler writes into those globals directly under the PLC scan lock.
- The web user interface (`webserver/webserver.py:61`) reads settings from `openplc.db` and drives runtime services via `openplc.runtime()` (`webserver/openplc.py:78`), which shells `./core/openplc` and proxies control commands through the interactive server socket.
- Master functionality (Modbus TCP/RTU polling) is entangled with the GUI: `webserver/webserver.py:123` emits `mbconfig.cfg`, while `webserver/core/modbus_master.cpp:21` parses that file at startup to spawn slave queries.

## 1. Isolate ICS Protocol Parsing from the Core Loop
**Current coupling**
- `main.cpp` owns lifecycle calls for Modbus server, Modbus master, Snap7, EtherCAT, and persistent storage. There is no abstraction for protocol services; everything shares the global PLC buffers.
- `server.cpp` and `modbus.cpp` operate directly on IEC memory, so the transport layer, parser, and application state updates are inseparable.
- The interactive server toggles protocol threads by mutating globals seen by both the runtime and the web UI, keeping orchestration logic outside the runtime main loop but without boundaries or dependency injection.

**Refactor direction**
- Introduce a protocol service interface (e.g. `struct ProtocolService { void start(const RuntimeContext&); void stop(); void poll(); };`) that receives an IO façade instead of raw globals. `main.cpp` can own a registry of enabled services, iterating them during initialization and shutdown. This removes the direct dependency on `initializeMB()` and friends inside the PLC loop.
- Split `server.cpp` into (a) a reusable TCP acceptor that emits byte spans, and (b) protocol adapters (Modbus, ENIP) that transform bytes into PLC operations through an injected `IOAdapter`. The Modbus handlers in `modbus.cpp` can then move into a `modbus::Parser` class that speaks in terms of `IRegisterBank` interfaces rather than the `ladder.h` arrays.
- Extract the IO binding logic (`mapUnusedIO()`, `updateBuffersIn_MB()`, etc.) into a dedicated “PLC image” module. Protocol implementations should depend on that module through narrow methods (`readDiscreteCoil(index)`, `writeHoldingRegister(index, value)`), enabling testing and future protocol swaps.
- Replace the interactive server globals with a controller object that owns protocol service instances. CLI or other frontends would call methods on that controller, preventing socket threads from poking shared globals.

**Quick wins**
- Create a thin wrapper around the IEC buffers (now called `IoFacade`) and migrate Modbus helpers to consume it. This is mostly mechanical but unlocks better separation.
- Move protocol-start logic out of `main()` into a `RuntimeServices` initializer that can be unit-tested without the scan loop.

### IoFacade map
The new `webserver/core/io_facade.h` defines the interface that protocol services should depend on. Each subsystem maps naturally onto its methods:

- `webserver/core/modbus.cpp:1107` → translate function codes to `readCoil`, `writeHoldingRegister`, etc., instead of touching `bool_output` / `int_output` directly.
- `webserver/core/modbus_master.cpp:693` and `webserver/core/modbus_master.cpp:711` → replace raw buffer synchronization with bulk reads/writes through `IoFacade`, allowing the master to operate on snapshots.
- `webserver/core/server.cpp:205` → thread entry points should accept an `IoFacade&` when they instantiate protocol handlers.
- `webserver/core/interactive_server.cpp:76` → the protocol controller can start threads by passing the shared `IoFacade` implementation rather than relying on globals.
- `webserver/core/main.cpp:106` → runtime bootstrap owns the concrete `IoFacade` implementation that binds onto the IEC arrays.

Future isolation work (e.g., seL4 endpoints) only has to swap out the concrete implementation of `IoFacade`—parsers and services already speak in terms of the abstract API.

### IoFacade adoption phases
1. **IEC-backed implementation** – introduce `LadderIoFacade` (name TBD) that wraps the existing `bool_input`, `bool_output`, `int_input`, and `int_output` arrays with bounds checking. Wire it into `main.cpp` and expose it to the protocol controller.
2. **Protocol migration** – adapt Modbus server/master, ENIP, and any remaining protocol handlers to depend solely on the facade. This removes direct includes of `ladder.h` from protocol-specific compilation units.
3. **Runtime controller** – replace the interactive server globals with a controller that owns a single `IoFacade` instance and summons protocol services. This is the natural place to add CLI hooks.
4. **Isolation layer** – once everything flows through the facade, swap the implementation for an IPC-backed variant (shared memory or seL4 messages). The protocol code remains untouched; only the facade implementation changes.
5. **Retirement of legacy glue** – after the facade is in place, delete unused helpers like `mapUnusedIO()` and remove direct buffer exports from `ladder.h`, ensuring all access is mediated.

## 2. Simplify Runtime Startup (CLI over Web UI)
**Current behavior**
- The Flask app in `webserver/webserver.py:61` controls protocol enablement, compiles IEC programs, and manages persistent storage. Configuration is persisted in SQLite and pushed into `mbconfig.cfg` and runtime RPCs.
- Runtime control happens via the `openplc.runtime` class (`webserver/openplc.py:78`), which shell-executes the binary, then forwards commands like `start_modbus(<port>)` or `quit()` to the interactive server (`interactive_server.cpp:149`).
- Compilation pipelines (`scripts/compile_program.sh`) expect to be invoked from the web context, wiring output back through the non-blocking stream reader in Python.

**CLI-friendly plan**
- Promote the interactive command set (start/stop services, load program, query status) into first-class CLI commands. A simple `openplcctl` tool can talk to the refactored controller directly (linking into the binary or reusing the localhost socket).
- Add argument parsing to `main.cpp` (or a small `runtime_cli.cpp`) so the PLC runtime can be launched with options like `--modbus-tcp 502` and `--program path/to/Config0.st`. That removes the need for the Flask layer to mediate configuration at startup.
- Migrate configuration storage away from SQLite/Flask by storing a simple TOML/YAML config consumed at boot. The new CLI would read/write that file rather than manipulating `openplc.db` tables.
- For compilation, retain `scripts/compile_program.sh` but expose it via CLI (e.g. `openplcctl compile program.st`). The CLI can still leverage existing scripts; no GUI callbacks are required.

**Decommissioning the web stack**
- Once CLI coverage exists, retire Flask routes, session management, and the `pages.py` templating. Runtime scripts (`openplc.py`) only need to stay if historical automation relies on them; otherwise replace with the new CLI calls.
- Remove the interactive server socket if the CLI is linked directly with the runtime controller. Alternatively, keep the socket but limit it to CLI clients and drop Flask user management and file upload logic.

## 3. Reduce Scope to Modbus TCP/RTU + Core Control
**Modules to keep**
- PLC scan loop (`webserver/core/main.cpp`) and hardware abstraction (`webserver/core/hardware_layer*.cpp`).
- Modbus server and master implementations (`webserver/core/modbus.cpp`, `webserver/core/modbus_master.cpp`) plus supporting utilities under `utils/libmodbus_src`.

**Modules to disable or remove**
- Other protocol stacks: Snap7 (`webserver/core/oplc_snap7.*`), EtherNet/IP (`webserver/core/enip.cpp`), PCCC, DNP3 (`webserver/core/dnp3*`, `utils/dnp3_src`), and EtherCAT hooks. These can be hidden behind feature flags or deleted to shrink the attack and maintenance surface.
- Web interface assets: Flask app, templates, static assets, and the SQLite schema (`webserver/webserver.py`, `webserver/pages.py`, `webserver/static`, `webserver/templates`, etc.).
- Persistent storage scheduler (`webserver/core/persistent_storage.cpp`) if it is only triggered from the Flask GUI, or reintroduce it later with a CLI-managed cadence.

**Build and configuration impact**
- Update build scripts (Makefile/CMake) to eliminate optional protocol sources so the binary does not link unused stacks. That includes striping `utils/dnp3_src`, Snap7 glue, and EtherCAT includes from the compilation pipeline.
- Prune configuration files generated by the GUI (`openplc.db`, `mbconfig.cfg` generators). Replace with a static config describing the limited Modbus endpoints you still support.
- Ensure `ladder.h` no longer exposes prototypes for removed subsystems. Stub implementations can live behind conditional compilation until the code is fully removed.

**Testing focus**
- Regression-test Modbus TCP and RTU flows using the new protocol interface to confirm IO updates still mirror PLC expectations.
- Validate the CLI-based startup covers the prior automation scenarios (start runtime, deploy IEC program, enable modbus server, query status, shutdown).

## Next Steps Checklist
- [ ] Implement a concrete `IoFacade` backed by the existing IEC buffers and refactor Modbus handlers to use it.
- [ ] Introduce a protocol service registry so `main.cpp` no longer hardcodes `initialize*` calls.
- [ ] Design and implement `openplcctl` (or equivalent) to replace Flask/SQLite orchestration.
- [ ] Remove unused protocol code paths, guarding anything temporarily required behind compile-time flags.
- [ ] Update build artifacts and documentation to reflect the streamlined, CLI-driven Modbus-only runtime.
