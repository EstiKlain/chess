# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Kung Fu Chess — a real-time variant of chess (no turns; pieces move independently with cooldowns).
Windows/MSVC + CMake (Visual Studio 17 2022 generator, pre-configured in `build/`). Two subsystems:

- **`core/`** — the original local game engine (Board/Rules/GameEngine/RealTimeArbiter/Controller/Renderer/Text I/O), driven by `chess_gui` (OpenCV window) and `chess_game` (text mode, legacy/frozen — see below).
- **`server/`** — a WebSocket multiplayer layer being built on top of `core/`, one Server-Iteration at a time. Full design doc: [docs/kungfu_chess_server_plan (1).md](docs/kungfu_chess_server_plan%20(1).md). The UI side has its own iteration doc: [docs/kungfu_chess_ui_plan.md](docs/kungfu_chess_ui_plan.md).

**Read the relevant plan doc before touching `src/server/` or planning the next server iteration** — it defines the iteration order, the ports/adapters layering, the wire protocol, and a decisions log that overrides anything that looks more "obvious" in code.

## Build / test

Build (Debug, from the pre-generated `build/` solution):
```
cmake --build build --config Debug
```
Or open `build/chess_game.sln` in Visual Studio.

Run all tests via CTest:
```
ctest --test-dir build -C Debug
```
Run a single doctest binary directly (faster iteration, supports doctest filters):
```
build/Debug/chess_tests.exe                      # core + view tests
build/Debug/chess_tests.exe --test-case="Board*"  # filter by name/wildcard
build/Debug/chess_server_tests.exe                # server-layer tests only
```
There are two independent test binaries/targets — **never mix them**:
- `chess_tests` (`tests/unit/*.cpp`) — core/view, links OpenCV.
- `chess_server_tests` (`tests/unit/server/*.cpp`) — server layer, links nlohmann_json + Threads, no OpenCV, no `core/` sources. Add new server tests here, with their own `doctest_main.cpp` (already present — don't add a second `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN`).

When adding a new `.cpp` file to `core/`, `server/`, or `tests/`, register it explicitly in the corresponding `add_executable(...)` source list in [CMakeLists.txt](CMakeLists.txt) — there is no glob.

Executables: `chess_game` (text mode, frozen — do not invest further here, see plan decisions log), `chess_gui` (OpenCV GUI, local play), `chess_server` (WebSocket server, port 9002), `img_test` (manual OpenCV smoke test).

## Architecture

### `core/` — the domain, shared by every consumer

`GameEngine` (`src/core/engine/GameEngine.hpp`) is the single gate in and out of the game: `requestMove`/`requestJump` in, `wait(ms)` to advance the clock, `snapshot() const` out (a read-only `GameSnapshot` value, never a live `Board&`). Nothing outside `GameEngine` touches `Board`/`RealTimeArbiter` directly — not `Controller`, not (eventually) any server use-case.

Fixed request pipeline inside `GameEngine`: `game_over` check → `motion_in_progress` check → `RuleEngine` → `RealTimeArbiter::startMotion`. Real-time concurrency (per-piece cooldowns/rests via `config::statsFor`) is `core/` domain logic and does not change when the server layer is added — see the plan's decisions log for the distinction between this and *network*-level concurrency (see below).

`core/` knows nothing about JSON, WebSocket, SQLite, matchmaking, or sessions — those only exist in `server/`. `src/config.hpp` holds all game-balance constants (cell size, per-piece speed/rest durations); don't hardcode numbers that belong there.

`src/view/` (renderer/canvas/animator/HUD input) only ever consumes `GameSnapshot`, never `Board`/`Piece` directly — see [docs/kungfu_chess_ui_plan.md](docs/kungfu_chess_ui_plan.md) for the full class table and the reasoning (Observer pattern for HUD subscribers so the hot `Controller.click → GameEngine` path stays synchronous and cheap).

### `server/` — WebSocket layer on top of `core/` (ports & adapters)

Directory layout mirrors dependency direction, arrows always point inward — `infrastructure/protocol → application → domain_ports → core`, never the reverse; `application/` never `#include`s anything from `infrastructure/`:

- `domain_ports/` — abstract interfaces only (`IEventBus`, `ITransport`, and future `ISessionStore`/`IClock`/etc per the plan).
- `application/` — use-cases; zero dependency on WebSocket/SQLite/JSON, only on `core/` + `domain_ports/`. (`ConnectionManager` here.)
- `infrastructure/` — concrete ports (`bus/InProcessEventBus`, `transport/WebSocketTransport`).
- `protocol/` — JSON envelope + DTOs (`dto/MessageEnvelope.hpp`) and `MessageRouter`, which parses raw JSON and either publishes a `BusEvent` on the `IEventBus` (valid message — the router never decides business logic for a given `type`) or replies `ERROR` directly via `ITransport` (malformed JSON only).
- `main_server.cpp` — the composition root: manual wiring only (no DI container), currently wires transport lifecycle → `ConnectionManager`, raw messages → `MessageRouter`, and a `PING`→`PONG` bus subscriber as the one demo "business logic" handler.

Wire protocol: every message is `{ "type", "requestId", "payload" }` in both directions (see the plan doc for the full message/error-code table). Error codes are strings (`ILLEGAL_MOVE`, `AUTH_REQUIRED`, etc.), never HTTP status codes.

Currently implemented: **Server-Iteration 1** only (transport + bus + envelope + PING/PONG round trip). Iterations 2–9 (real moves, login, logging, reconnect, SQLite/Elo, matchmaking, rooms, sound/animation subscribers) are planned but not yet built — check the plan doc's iteration list before assuming any of that exists.

Test doctrine for `server/` (per the plan): every use-case is tested with the **real** `core/` plus **fake** ports (fake `IEventBus`, fake `IClock`, etc.) — never real WebSocket/SQLite in a unit test. Real IO is only exercised under `tests/integration/`.

One thing to keep in mind when eventually implementing `GameSession`/`MakeMoveUseCase` (Iteration 2): network-level concurrency (multiple WS connections calling into the same `GameEngine` from different threads) is a distinct concern from game-rule concurrency (piece cooldowns) — the plan requires serializing access per-session (e.g. a mutex per `GameSession`) from Iteration 2 onward, and `ConnectionManager` is expected to be a `connectionId → (GameSession, role)` map from Iteration 2 onward even while only one session exists in practice.
