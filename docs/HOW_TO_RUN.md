# How to run this project (verified, exact)

Single source of truth for **building, testing and running** Kung Fu Chess on this machine.
Everything below was executed and verified on 2026-07-28 (Windows 11 Pro, VS 2022 Professional,
MSVC 19.41.34120, CMake 3.29 bundled with VS). Anything not verified is marked
`[NOT VERIFIED]`.

Read this file **before** trying to build or run anything. Do not re-derive the commands.

---

## 0. TL;DR — the four commands that matter

```bash
"C:/Program Files/Microsoft Visual Studio/2022/Professional/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" --build E:/Bootcamp/chess/build --config Debug
```

```bash
"C:/Program Files/Microsoft Visual Studio/2022/Professional/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/ctest.exe" --test-dir E:/Bootcamp/chess/build -C Debug
```

```bash
E:/Bootcamp/chess/build/Debug/chess_server.exe
```

```bash
E:/Bootcamp/chess/build/Debug/chess_gui.exe
```

Order for actual play: build → start `chess_server` (one) → start `chess_gui` (two, each in its
own console window).

---

## 1. Prerequisites (already satisfied on this machine)

| Requirement | State here | Notes |
|---|---|---|
| Visual Studio 2022 **Professional** | installed | `C:\Program Files\Microsoft Visual Studio\2022\Professional` — the only 2022 edition installed. |
| CMake ≥ 3.14 | **bundled with VS, NOT on `PATH`** | Typing `cmake` in a plain PowerShell/bash shell fails with "not recognized". Use the full path (§0) or a shell where it is on `PATH`. |
| `ctest` | same directory as `cmake.exe` | |
| Git | required | All third-party deps are fetched by `FetchContent` at configure time via `git clone`. |
| OpenCV 4.5.1 prebuilt | present at `OpenCV_451/` | **`OpenCV_451/` is in `.gitignore`** — a fresh clone will NOT have it, and CMake will fail to find headers/libs. Layout expected by [CMakeLists.txt](../CMakeLists.txt): `OpenCV_451/include/` (headers) and `OpenCV_451/bin/` (`opencv_world451.dll`, `opencv_world451d.dll`, `opencv_world451.lib`, `opencv_world451d.lib`). |
| Internet access | only for a **fresh** configure | Deps are cached under `build/_deps` afterwards. |

Auto-fetched dependencies (no manual install): asio `asio-1-16-1`, websocketpp `0.8.2`,
nlohmann/json `v3.11.3`, libsodium (`libsodium-cmake`, `master`).

The OpenCV DLL is copied next to every OpenCV-linking executable by a `POST_BUILD` step — you
never copy it by hand.

---

## 2. Configure (only if `build/` is missing or `CMakeCache.txt` is broken)

`build/` is **already configured and committed-to-disk** (Visual Studio 17 2022 generator, x64).
Normally you skip this step entirely and go straight to §3.

Fresh configure, if ever needed:

```bash
"C:/Program Files/Microsoft Visual Studio/2022/Professional/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" -S E:/Bootcamp/chess -B E:/Bootcamp/chess/build -G "Visual Studio 17 2022" -A x64
```

Verified: a from-scratch configure (all four deps cloned) took **~290 seconds**. It is normal for
this to look frozen for minutes while libsodium is fetched.

> **Critical gotcha — keep the build directory path SHORT.**
> Configuring into a long path (e.g. under `C:\Users\...\AppData\Local\Temp\claude\...\freshbuild`)
> **fails**, verified, with:
> `error : unable to create file tests/reports/2016-09-09-nativejson_benchmark/... : Filename too long`
> while cloning nlohmann/json, followed by `MSB8066` and
> `CMake Error ... Build step for json failed: 1`.
> This is Windows' `MAX_PATH` limit hitting a deeply nested file inside the json repo, not a
> project bug. Use a short build path such as `E:/Bootcamp/chess/build` (verified working), or
> enable long paths (`git config --system core.longpaths true`) `[NOT VERIFIED]`.

`BUILD_TESTS` is `ON` by default; leave it on.

---

## 3. Build

```bash
"C:/Program Files/Microsoft Visual Studio/2022/Professional/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" --build E:/Bootcamp/chess/build --config Debug
```

Builds **all** targets. Output goes to `build/Debug/`:

| Executable | What it is |
|---|---|
| `chess_server.exe` | The WebSocket game server. Listens on **port 9002**. Owns the only real `GameEngine`. |
| `chess_gui.exe` | The OpenCV GUI client. **Networked only** — it has no local engine; it cannot play without a running server. |
| `chess_game.exe` | Legacy text-mode engine (frozen, local only, reads a script from stdin). Not part of networked play. |
| `chess_tests.exe` | core/ + view/ unit tests (links OpenCV). |
| `chess_server_tests.exe` | server/ unit tests (no OpenCV, no real sockets). |
| `chess_client_tests.exe` | client_net/ unit tests (no OpenCV, no real sockets). |
| `img_test.exe` | Manual OpenCV smoke test. |

Notes:
- Hundreds of `C4267` warnings from `websocketpp`/`asio` headers are expected and harmless.
- **A build can fail once and succeed on an immediate re-run with zero source changes** —
  observed here (and previously documented in `CLAUDE.md` for `RealTimeArbiter.cpp`). It is a
  parallel-build race, not a real error. **If the build fails, run the exact same command a
  second time before investigating.**
- Use `--config Release` instead of `Debug` for a release build `[NOT VERIFIED]`; every path in
  this document assumes `Debug`.
- Adding a new `.cpp` requires registering it explicitly in the right `add_executable(...)` list
  in [CMakeLists.txt](../CMakeLists.txt) — there is no glob.

---

## 4. Test

All three test binaries via CTest:

```bash
"C:/Program Files/Microsoft Visual Studio/2022/Professional/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/ctest.exe" --test-dir E:/Bootcamp/chess/build -C Debug
```

Verified output: `100% tests passed, 0 tests failed out of 3` (`chess_tests`,
`chess_server_tests`, `chess_client_tests`), total ~1.5 s.

Faster iteration — run one doctest binary directly (supports doctest filters):

```bash
E:/Bootcamp/chess/build/Debug/chess_server_tests.exe
```

```bash
E:/Bootcamp/chess/build/Debug/chess_tests.exe --test-case="Board*"
```

Never mix test targets: core/view tests go in `tests/unit/`, server tests in
`tests/unit/server/`, client tests in `tests/unit/client_net/` — each has its own
`doctest_main.cpp` already.

---

## 5. Run a real game (server + two GUI clients)

### 5.1 Start the server — always first

```bash
E:/Bootcamp/chess/build/Debug/chess_server.exe
```

Expected on stdout: `[server] listening on port 9002...` and then one
`[server] connection opened: <id> as 'w' (total: 1)` line per client.
Verified: the process binds `0.0.0.0:9002` and `[::]:9002` (LISTENING), i.e. all interfaces, so
LAN play works without changes — clients on other machines just need this machine's IP and a
Windows Firewall rule allowing inbound TCP 9002 `[NOT VERIFIED]`.

The server:
- loads `assets/opening_board.txt` (standard 8×8 Kung Fu Chess opening) via `PROJECT_ROOT`,
- runs a 50 ms tick thread that advances the game clock and broadcasts `STATE_UPDATE` to every
  connection continuously,
- accepts exactly **two** players; a third connection is rejected immediately with `TABLE_FULL`,
- has **no graceful shutdown** — stop it with Ctrl+C / kill the process.

### 5.2 Start each client in its own console window

```bash
E:/Bootcamp/chess/build/Debug/chess_gui.exe
```

Run it **twice**, from two separate terminal windows (or double-click it twice in Explorer).
It calls `AllocConsole()` and prompts, in order:

```
Server host [localhost]:      <- Enter = localhost; otherwise the server machine's hostname/IP
Username:                     <- any name; typed in plain text
Password:                     <- typed in plain text, NOT masked
```

- **Login auto-registers**: an unknown username creates the account with that password and Elo
  1200 (`InMemoryUserRepository` + Argon2id via libsodium). A known username with the wrong
  password is rejected with `INVALID_CREDENTIALS` and the client exits with code 1.
- Accounts live **in memory only** — restarting `chess_server` wipes every account and Elo.
  PostgreSQL is written but deliberately not compiled/wired yet (see `CLAUDE.md`, Iteration 6).
- If the server is not running, the client prints `Failed to connect to <host>: ...` and exits 1.
- The OpenCV window opens only **after** a successful login and the first `STATE_UPDATE`; its
  size comes from the server's snapshot, not a constant.

> **Automation warning (verified previously, still true):** `chess_gui` cannot be driven by piped
> or scripted stdin. `AllocConsole()` + `freopen_s(..., "CONIN$", ...)` rebinds stdin to the new
> console's keyboard, so a piped `echo ... | chess_gui.exe` just hangs at the first prompt. Any
> end-to-end GUI check needs a human at a real keyboard/mouse. An LLM must not claim a two-window
> manual test was performed unless a human ran it.

### 5.3 Controls

- **Left click** a piece = select it; **left click** a destination = request the move.
  Left-clicking another piece of the same color re-selects instead of moving.
- **Right click** a cell = jump request for the piece there.
- Illegal requests are rejected by the server (`ILLEGAL_MOVE`) and simply do nothing on screen.
- Real-time rules: no turns; each piece has its own speed/cooldown from `src/config.hpp`.
- Known gap, not a bug: **color ownership is not enforced** — either player can currently move
  either color's pieces.
- Disconnecting a client starts a 20 s countdown HUD; the client auto-reconnects within that
  window, otherwise the disconnected player is resigned and Elo is applied.

---

## 6. Run the legacy text mode (`chess_game`) — local, no server

It reads an entire script from **stdin** and exits. Board section, then commands section:

```bash
printf 'Board:\nwK . .\n. . .\n. . bK\nCommands:\nclick 50 50\nclick 50 150\nwait 2000\nprint board\n' | E:/Bootcamp/chess/build/Debug/chess_game.exe
```

Verified output:

```
. . .
wK . .
. . bK
```

Command verbs: `click <x> <y>`, `jump <x> <y>`, `wait <ms>`, `print board`. Coordinates are
**pixels**, not cells — cell size is 100 px, so cell `(row, col)` is `x = col*100 + 50`,
`y = row*100 + 50`. This target is frozen; don't invest in it.

---

## 7. Testing the server without any GUI (protocol-level)

Useful for an LLM/agent that cannot use a mouse. Connect any WebSocket client to
`ws://localhost:9002` and send JSON. Every message, both directions, is
`{ "type", "requestId", "payload" }` (`requestId` and `payload` are optional on input).

```json
{"type":"LOGIN","requestId":"1","payload":{"username":"alice","password":"pw"}}
```

```json
{"type":"MOVE","requestId":"2","payload":{"fromRow":6,"fromCol":0,"toRow":5,"toCol":0}}
```

```json
{"type":"JUMP","requestId":"3","payload":{"row":6,"col":1}}
```

```json
{"type":"PING","requestId":"4","payload":{}}
```

- Everything except `LOGIN`/`PING` requires a successful `LOGIN` first, otherwise `AUTH_REQUIRED`.
- After login you receive a `STATE_UPDATE` roughly every 50 ms, forever — that stream is normal,
  not a bug.
- Coordinates are **raw integers** (`row` 0 = top/black side). There is no algebraic (`"e2"`)
  notation anywhere in this codebase.
- Error codes are strings: `AUTH_REQUIRED`, `ILLEGAL_MOVE`, `MALFORMED_PAYLOAD`, `TABLE_FULL`,
  `INVALID_CREDENTIALS`, `INTERNAL_ERROR`, `SESSION_EXPIRED`.

---

## 8. Paths, logs and generated files

- `PROJECT_ROOT` is compiled in as an **absolute** path (`E:/Bootcamp/chess`). The working
  directory you launch from is therefore irrelevant — assets and logs always resolve to the
  repository, never to `.`.
- `logs/server.log` — every message in/out of the server, appended, `LOGIN` passwords redacted
  to `"***"`.
- `logs/client.log` — same, per client, but **every `chess_gui` process on this machine writes to
  this one shared file** (the path comes from `PROJECT_ROOT`, not the working directory).
- Both logs grow continuously because of the 50 ms broadcast. **There is no rotation or size
  cap** — delete them manually if they get large.
- `logs/` and `build/` are gitignored.

---

## 9. Troubleshooting (all observed here)

| Symptom | Cause / fix |
|---|---|
| `cmake : The term 'cmake' is not recognized` | CMake is not on `PATH`. Use the full VS-bundled path from §0. |
| Build fails once, no source change, succeeds on re-run | Parallel-build race. Re-run the same command before investigating. |
| `Filename too long` / `MSB8066` while cloning json | Build directory path is too long. Use `E:/Bootcamp/chess/build` (§2). |
| `Failed to connect to localhost: ...` in the client | `chess_server` is not running, or was stopped. Start it first. |
| Client rejected with `TABLE_FULL` | Two players are already seated, **or** a disconnected player's seat is still reserved (20 s window). Restart the server to clear all seats. |
| Second move always `ILLEGAL_MOVE` / `motion_in_progress` | The previous motion hasn't resolved. Only the server's tick thread resolves motions — if it isn't running, the game is stuck by design. |
| Login rejected with `INVALID_CREDENTIALS` | The username exists in memory with a different password. Use the original password or restart the server (accounts are in-memory only). |
| Piping input into `chess_gui` hangs | Expected — see §5.2. Only real keyboard input works. |
| Missing `opencv_world451d.dll` at runtime | Run the executable from `build/Debug/` (the POST_BUILD copy puts the DLL there), or rebuild. |

---

## 10. What does NOT exist yet (don't look for it)

No matchmaking, no rooms, no lobby, no PostgreSQL (`PostgresUserRepository.*` exists as source
but is **deliberately not registered in CMake** and does not compile), no Docker image, no
graceful server shutdown, no turn/color ownership enforcement, no sound/animation bus
subscribers. See `CLAUDE.md` and `docs/kungfu_chess_server_plan (1).md` for the iteration plan
and the reasoning behind each deferral.
