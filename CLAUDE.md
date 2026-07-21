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
- `chess_server_tests` (`tests/unit/server/*.cpp`) — server layer, links nlohmann_json + Threads, no OpenCV. Add new server tests here, with their own `doctest_main.cpp` (already present — don't add a second `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN`). **Updated in Iteration 2:** this target now also links real `core/` sources (`Board`, `Movement`, `RuleEngine`, `GameOverRule`, `PieceRules`, `RealTimeArbiter`, `BoardParser`, `BoardPrinter`, `GameEngine`) — the plan's test doctrine requires use-case tests to run against a **real** `GameEngine`, only the ports (`IEventBus`/`ITransport`) are faked. `core/` has no OpenCV dependency of its own (only `view/`/`app/` do), so OpenCV still never enters this target.

When adding a new `.cpp` file to `core/`, `server/`, or `tests/`, register it explicitly in the corresponding `add_executable(...)` source list in [CMakeLists.txt](CMakeLists.txt) — there is no glob.

Executables: `chess_game` (text mode, frozen — do not invest further here, see plan decisions log), `chess_gui` (OpenCV GUI, local play), `chess_server` (WebSocket server, port 9002), `img_test` (manual OpenCV smoke test).

## Comment & commit conventions (user directive, applies from Server-Iteration 2 onward)

- All comments and commit messages: **English only**, no exceptions.
- Comments explain **WHY**, never WHAT — the code already says what it does via naming. Don't restate the obvious ("increments counter"); do explain a non-obvious constraint, a workaround, or a decision that would surprise a reader.
- Same rule for commit messages: describe the motivation/reason for the change, not a restatement of the diff.
- **Exception**: every function declared in a `.hpp` file gets a short Doxygen-style comment (`/** ... */` with `@param`/`@return` as relevant) directly above it describing what it does. This is the one place a "what" comment is required — it's API documentation for callers, not an inline comment about implementation.

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

Currently implemented: **Server-Iterations 1–2** (transport + bus + envelope + PING/PONG, and real `MOVE`/`JUMP` against a real `GameEngine` with `STATE_UPDATE` fan-out). Iterations 3–9 (login, logging, reconnect, SQLite/Elo, matchmaking, rooms, sound/animation subscribers) are planned but not yet built — check the plan doc's iteration list before assuming any of that exists. There is also still no networked client (`client_net/ServerConnection`) — `chess_gui` still plays entirely locally; "playing" over the network today means sending/receiving the raw `MOVE`/`JUMP`/`STATE_UPDATE` JSON directly, the same way Iteration 1's `PING`/`PONG` was demoed.

Test doctrine for `server/` (per the plan): every use-case is tested with the **real** `core/` plus **fake** ports (fake `IEventBus`, fake `IClock`, etc.) — never real WebSocket/SQLite in a unit test. Real IO is only exercised under `tests/integration/`.

Network-level concurrency (multiple WS connections calling into the same `GameEngine` from different threads) is a distinct concern from game-rule concurrency (piece cooldowns) — `GameSession` serializes access with its own `std::mutex` from Iteration 2 onward, and `ConnectionManager` is a `connectionId → (GameSession, role)` map from Iteration 2 onward even while only one session exists in practice.

#### Server-Iteration 1 — design review (done, verified green)

Reviewed file-by-file against the plan and confirmed: dependency direction holds (`MessageRouter`/`main_server` only ever touch `IEventBus`/`ITransport`, never `InProcessEventBus`/`WebSocketTransport` directly), `MessageEnvelope` + `WebSocketTransport` are the only two places that see raw JSON/sockets, `ConnectionManager` is a pure registry (no session/color concept yet, as required), and `chess_server_tests` uses fakes for both ports, never a real socket. Manually smoke-tested (browser `WebSocket` to `ws://localhost:9002`, sent `PING`, received `PONG`) — round trip confirmed working. Iteration 1 is complete and committed.

Two non-blocking design notes to keep in mind for Iteration 2+ (don't repeat the pattern, but not worth reworking Iteration 1 for):
- `main_server.cpp` currently builds the `PONG` JSON inline in the `bus.subscribe("PING", ...)` lambda — real business logic sitting in the composition root, which per the layer-ownership table should hold wiring only. Acceptable as a minimal wiring-proof for Iteration 1; from Iteration 2 on, real message handling (`MOVE`, etc.) must live in `application/` use-cases, not as lambdas in `main_server.cpp`.
- `MessageRouter::handleRawMessage` puts the raw `std::exception::what()` text into the `ERROR` payload sent back over the network — a minor internal-detail leak to an untrusted peer. Harmless at this stage; swap for a generic message when this code path is next touched.

#### Server-Iteration 2 — plan review (before implementation)

Reviewed the proposed Iteration 2 plan (GameSession/MakeMoveUseCase/mappers/DTOs) against `docs/kungfu_chess_server_plan (1).md` and the actual `core/` signatures. Overall shape is sound (mapper as the single core+protocol touchpoint, per-session mutex, direct send-to-both instead of broadcast, connection-time third-player rejection, `ConnectionManager` as a map from day one). Two corrections made before implementation started:

- **`JUMP` is not `MOVE`-shaped.** `GameEngine::requestJump(int row, int col)` (`src/core/engine/GameEngine.hpp`) takes a single board position, not a `{from, to}` pair like `requestMove(const MoveRequest&)`. The plan's "JUMP symmetric to MOVE, reuse the mapper" wording was wrong for the wire shape — `JUMP` payload is `{ "pos": "e4" }`, its own `JumpDto`, mapped by its own function (not `MoveRequestMapper`) straight to `(row, col)` ints, not a `MoveRequest`.
- **`STATE_UPDATE`'s `role` field is per-recipient, not shared.** `GameSnapshot` itself is one shared value, but the two connections in a session see different `role`/color. `GameSnapshotMapper::toJson` takes the color as a parameter (`toJson(snapshot, color)`), and `MakeMoveUseCase` builds two distinct JSON payloads (one per connectionId) — never one shared string sent via `broadcast`.

**Wire shape corrected during review (user caught this):** the plan's protocol table used algebraic strings (`"e2"`/`"e4"`) for `MOVE`/`JUMP` payloads, but no algebraic-notation parsing exists anywhere in `core/`/`view/` today — `BoardMapper::pixelToCell` (`src/core/input/BoardMapper.cpp`) already converts pixels straight to numeric `Position{row, col}`, with no letter/file step at any point. Introducing "e4" parsing at the mapper would be new, untested translation logic serving no existing producer. **Decision: DTOs carry raw integers, matching `core/`'s own representation** — `MoveDto{fromRow,fromCol,toRow,toCol}`, `JumpDto{row,col}` (`JumpDto` deliberately does NOT reuse `MoveDto` — see the `requestJump(row,col)` signature note above).

#### Server-Iteration 2 — implementation complete (done, verified green)

Built `GameSession`, `ConnectionManager` (now a map with colors + `ConnectionOutcome`), `MakeMoveUseCase`, `MoveRequestMapper`, `GameSnapshotMapper`, `MoveDto`/`JumpDto`/`StateUpdateDto`, and `protocol::envelope`/`protocol::errorEnvelope` (`src/server/protocol/Envelope.hpp`). `main_server.cpp`'s `bus.subscribe("MOVE"/"JUMP", ...)` lambdas are one-line delegations to `MakeMoveUseCase` — the Iteration 1 note about business logic sitting in the composition root is fixed for these two message types. 18 server-layer tests (`Mappers`/`MakeMoveUseCase`/`ConnectionManager`, real `GameEngine` + fake ports) and all 188 pre-existing `chess_tests` (core untouched) pass.

**Caught in code review, fixed before commit:** `errorEnvelope` originally only covered the `ERROR` shape, but `{"type", "requestId", "payload"}` is the wire-format envelope every outgoing message shares — not an ERROR-specific concept. Both `PONG` (`main_server.cpp`) and `STATE_UPDATE` (`MakeMoveUseCase::fanOutStateUpdate`, renamed from `broadcastStateUpdate` — it never called `ITransport::broadcast()`, that name was misleading) were each hand-rolling that same three-key JSON object inline, the exact duplication `errorEnvelope` already existed to avoid for `ERROR`. Renamed the file `ErrorEnvelope.hpp` → `Envelope.hpp`, added a generic `protocol::envelope(type, payload)`, and reimplemented `errorEnvelope` on top of it; `PONG` and `STATE_UPDATE` now both call `protocol::envelope(...)` instead of building the triple by hand. `PING` deliberately did **not** get its own use-case class (unlike `MOVE`/`JUMP`) — there is no `core/`/domain logic behind it at all, just an envelope, so a `PingUseCase` would be pure ceremony.

**Also fixed in the same review pass:** `MakeMoveUseCase` was sending `INTERNAL_ERROR` for two genuinely different failure classes — a malformed `MOVE`/`JUMP` payload (bad input **from the peer**) and a connection with no session binding at all (a state that should never happen **on our side**). Split into two codes: `MALFORMED_PAYLOAD` for the former (chosen over `BAD_PAYLOAD` to match the "malformed" wording already used in both this error's own message text and `MessageRouter`'s envelope-level parse error — code and message should use the same word for the same failure), `INTERNAL_ERROR` kept only for the latter. Not in the plan's original error-code table (`AUTH_REQUIRED`, `ILLEGAL_MOVE`, `NO_MATCH_FOUND`, `ROOM_NOT_FOUND`, `SESSION_EXPIRED`, `INTERNAL_ERROR`) — like `TABLE_FULL`, a new code added because a case came up in code review that the table didn't anticipate.

**Deferred, deliberately:** `ConnectionManager` holds a raw `GameSession*` per connection with no ownership model — safe today only because exactly one `GameSession` exists for the whole process lifetime (a `main()`-local variable). This becomes a real question once Iteration 7 (matchmaking) starts creating/destroying `GameSession`s dynamically — decide the ownership story (who constructs/destroys sessions, `unique_ptr` vs. a session registry in the composition root) then, not now; solving it today would be speculative design for a lifecycle that doesn't exist yet.

Skipped, deliberately, as unneeded abstraction: a separate `JumpRequestMapper` file. Unlike `MoveDto → MoveRequest` (a real shape change: flat fields → two `Position`s), `JumpDto{row,col}` already matches what `GameEngine::requestJump(row,col)` takes — there is no domain type in between to map to, so `MakeMoveUseCase` reads `dto.row`/`dto.col` directly. Also skipped: a standalone `GameSession.tests.cpp` — its `requestMove`/`requestJump`/`snapshot` forwarding is already exercised (with a real `GameEngine`) through the `MakeMoveUseCase` tests, so a separate file would just duplicate the same calls without a fake in between.

**Bug found only by manual testing, not by the unit tests (worth remembering why):** the first manual two-tab test showed the first `MOVE` succeed, but the second (a completely legal move for the other color) came back `ILLEGAL_MOVE`/`motion_in_progress` forever after. Cause: `GameEngine::wait(ms)` — the call that resolves in-flight motions/cooldowns — was never being called anywhere server-side. `chess_gui` resolves this itself via its own render loop (`src/app/main_gui.cpp`'s `while (!canvas.shouldClose())`, which calls `engine.wait(deltaMs)` every frame *before* drawing) — but the server has no render loop at all (drawing happens client-side, off `STATE_UPDATE`), so nothing was ever advancing the clock. The unit tests didn't catch this because each `MakeMoveUseCase` test only issues one move per `GameSession` — they never exercised a second, sequential move needing the first one's motion to have resolved first.

**Fix:** a detached `std::thread` in `main_server.cpp`, started right before `transport.run()`, sleeping `server_config::kTickIntervalMs` (50ms) and calling `session.wait(deltaMs)` each iteration — the server-side equivalent of `chess_gui`'s per-frame `engine.wait(deltaMs)` call, minus the drawing. This is **not** new game logic and does not touch `core/` — it is the same pre-existing `GameEngine::wait` function, just with a periodic caller added on the server side (where none existed). No `IClock` port was introduced for this — the plan already scopes `IClock` to Iteration 5 (disconnect timers), and this tick has nothing to do with that; it is unconditional real-wall-clock ticking, not something a test needs to fast-forward.

Why the client keeps its *own* separate loop instead of this being "the same work twice": the server's tick decides **when a motion is actually done** (game-state truth, lives wherever the one real `GameEngine` lives). A future networked client's loop would decide **how to draw the in-between frames of an already-started motion** smoothly (using `PieceSnapshot::motion`'s `fromRow/fromCol/toRow/toCol/startMs/durationMs` to interpolate against the client's own clock) between the discrete `STATE_UPDATE` messages it receives — `STATE_UPDATE` only arrives when a move starts, not every animation frame, so without its own loop the client would have pieces "teleport" instead of slide. Two different responsibilities (deciding vs. drawing), never the same computation in two places.

Manually verified end-to-end over a real WebSocket (browser `WebSocket` to `ws://localhost:9002`, real `assets/opening_board.txt`, two connections): first connection got `'w'`, second got `'b'`, a third was rejected immediately with `TABLE_FULL`; an illegal move (moving onto a friendly piece) returned `ILLEGAL_MOVE`/`friendly_destination` to the sender only; two *sequential* legal moves (one per color, waited out between them) each produced a `STATE_UPDATE` — with the correct per-recipient `role` — delivered to **both** connections, and the snapshot correctly showed one piece already `Idle` at its destination and the other still `Moving` with in-flight `motion` data. Iteration 2 is complete and green.
