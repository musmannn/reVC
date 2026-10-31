# Multiplayer: project context for contributors and agents

Read this first if you're working on reVC multiplayer. It's the single place
for where things stand, what's been decided, and where the relevant code is.
Keep it up to date as work lands.

## Documents

| Doc | What it is |
|---|---|
| [Design spec](../superpowers/specs/2026-10-31-multiplayer-design.md) | What we're building and why: goals, non-goals, anti-goals, constraints. The source of truth for scope. |
| [Plan 1: network foundation](../superpowers/plans/2026-10-31-mp-1-network-foundation.md) | Protocol, invite codes, relay server, fake client, CI. Needs no game files, so a cloud session can do all of it. |
| [Plan 2: free roam](../superpowers/plans/2026-10-31-mp-2-free-roam.md) | Game integration: menu, remote players, vehicles, combat, chat, free-roam menu. Compiles in CI; in-game checks need the game files. |
| [Plan 3: races](../superpowers/plans/2026-10-31-mp-3-races.md) | Race rules (unit tested), race protocol, in-game race flow, route recorder, 4 routes. |
| [Relay setup](relay-setup.md) | Running `revc-relay` on a cloud VPS or a home PC. |
| `docs/multiplayer/testing.md` | In-game manual test checklist (created in Plan 2). |

## Status

| Milestone | Scope | State |
|---|---|---|
| 1 | Invite-code sessions, free roam, races | Plan 1 Tasks 1–7 done (protocol, invite codes, clock, pose buffer, relay, net client, fake client, 51 tests). Next: Plan 1 Task 8 (CI) |
| 2 | Deathmatch, team deathmatch, scoreboards | Not started |
| 3 | More modes, race editor, dedicated server | Not started |

## Decisions log

Owner decisions are final unless the owner changes them. Builder calls can be
revisited with a reason.

| Date | Decision | By |
|---|---|---|
| 2026-10-31 | Friends join with an **invite code** through a small lobby/relay server (no port forwarding). | Owner |
| 2026-10-31 | **A player hosts**; the host is authoritative. No dedicated server in milestone 1. | Owner |
| 2026-10-31 | Milestone 1 = **free roam + races**. | Owner |
| 2026-10-31 | **Ambient traffic/peds stay local** per player; only players, their vehicles and mode objects sync. | Owner |
| 2026-10-31 | Relay runs on **both** a cheap/free cloud VPS (default address built in) and the owner's own PC (switch via a *Relay server* menu setting). | Owner |
| 2026-10-31 | **macOS, Linux and Windows** are all first-class with cross-play; CI builds and tests on all three. | Owner |
| 2026-10-31 | Free roam gives a **spawn loadout + weapon/health/armour menu**; no world pickups or Ammu-Nation in milestone 1. | Owner |
| 2026-10-31 | Race results ordered by racers' own crossing timestamps on a shared session clock, not by packet arrival. | Builder call |
| 2026-10-31 | Up to 8 players per session. Story script does not run in multiplayer. | Builder call |
| 2026-10-31 | Remote players are extra ped entities fed by the network. The local player stays the only `CPlayerPed`. | Builder call |
| 2026-10-31 | Remote players are `CCivilianPed` puppets with `CharCreatedBy = UNK_CHAR`, so the engine skips their AI and physics and they are never culled. | Builder call |
| 2026-10-31 | The multiplayer world starts from upstream `gamefiles/data/freeroam_miami.scm` instead of `main.scm`. Any pickups it places are local-only, like ambient traffic. | Builder call |
| 2026-10-31 | Shooter-side hit detection, victim-side damage. Remote players' shots are visual only on your screen. | Builder call |
| 2026-10-31 | Remote players' enter/exit-vehicle is shown as a warp in milestone 1; the local player keeps the full animation. | Builder call |
| 2026-10-31 | Engine-free code lives in `mp/` (static lib `mpcommon`, relay, tools, tests) so it builds and tests without game files. | Builder call |
| 2026-10-31 | A failed *create* (wrong version, bad name, relay full) is answered with `MSG_JOIN_RESULT` and the matching `eJoinResult`, so the client reports it through `OnJoinFailed`. The plan didn't define a create-failure message. | Builder call |
| 2026-10-31 | The relay checks the protocol version before parsing the rest of create/join, so a future layout still gets a clean `JOIN_VERSION_MISMATCH`. | Builder call |
| 2026-10-31 | Anti-spoofing: the relay never forwards relay-control types (below `MSG_CHAT`), and clients only accept them from `MP_PLAYER_RELAY`. `SendGame` only sends types ≥ `MSG_GAME_FIRST`, up to `MP_MAX_PACKET − 2` bytes (the envelope takes 2). | Builder call |
| 2026-10-31 | `MSG_PLAYER_LEFT.reason` is `eLeaveReason` (`LEAVE_QUIT` / `LEAVE_LOST`). Clients send ENet disconnect data `MP_DISCONNECT_QUIT` when leaving on purpose. | Builder call |
| 2026-10-31 | Duplicate names are compared case-insensitively. The base name is shortened so `name (n)` fits in 23 chars. | Builder call |
| 2026-10-31 | Ping/pong are unreliable. `CSessionClock` ignores samples with a negative RTT or one above 10 s, and `RttMs()` is the latest sample's RTT. | Builder call |
| 2026-10-31 | The relay drops connections that don't send create/join within 10 s. The ENet peer timeout is 5–15 s, so a crashed host ends its session within 15 s. | Builder call |
| 2026-10-31 | Added `mp/common/NetInit.*` (ref-counted `enet_initialize`), which the relay and `CNetClient` share. This file isn't in the plan's file list. | Builder call |
| 2026-10-31 | On `Stop()` the relay drains for 250 ms and then destroys the host without forcing disconnects. ENet discards a peer's undelivered packets when a disconnect arrives, which would swallow `SESSION_ENDED`. | Builder call |
| 2026-10-31 | Received chat text and names are filtered to printable ASCII in `CNetClient` before they reach the game. | Builder call |

## Ground rules

- **Single player must be untouched.** All multiplayer code goes behind
  `#ifdef MULTIPLAYER` (defined in `src/core/config.h`). This is also the
  upstream rule: the README says custom code must be wrapped in preprocessor
  conditions. With no session active, every multiplayer hook is a no-op.
- Follow `CODING_STYLE.md` and the surrounding code: C-style C++, `CName`
  classes, static manager classes with `Init/Update/Shutdown`, tabs.
- Third-party libraries are vendored under `vendor/` and wired into both
  CMake (`CMakeLists.txt`, `src/CMakeLists.txt`) and premake (`premake5.lua`).
- Testing in the real game needs original GTA Vice City files. They are
  **not** in the repo. Ask the owner for the install path if you need it.

## Code map

Where the engine touches what multiplayer needs.

| Concern | Where |
|---|---|
| Main per-frame loop (net send/receive hook point) | `CGame::Process()` in `src/core/Game.cpp` |
| Game init / start / shutdown | `CGame::Initialise`, `CGame::ReInitGameObjectVariables`, `CGame::ShutDown` in `src/core/Game.cpp` |
| "The player" lookups (~1,000 call sites, 67 files) | `FindPlayerPed`, `FindPlayerVehicle`, `FindPlayerCoors` in `src/core/PlayerInfo.cpp` |
| Player slots | `CWorld::Players[NUMPLAYERS]`, `CWorld::PlayerInFocus` in `src/core/World.h`; `NUMPLAYERS = 1` in `src/core/config.h` |
| Player state (money, wasted/busted) | `CPlayerInfo` in `src/core/PlayerInfo.h` |
| Ped classes | `CPed` (`src/peds/Ped.h`), `CPlayerPed` (`src/peds/PlayerPed.h`), `CCivilianPed` (`src/peds/CivilianPed.h`) |
| Ped spawning / cleanup (keep remote players safe from it) | `CPopulation` in `src/peds/Population.h` (`ManagePopulation`) |
| Vehicles | `src/vehicles/` (`CVehicle`, `CAutomobile`, `CBike`, `CBoat`) |
| Random traffic | `CCarCtrl::GenerateRandomCars` in `src/control/CarCtrl.cpp` |
| Input | `CPad` in `src/core/Pad.h` |
| Story script (must not run in MP) | `CTheScripts::Init/Process` in `src/control/Script.cpp` |
| Frontend menus (add Multiplayer page) | `src/core/MenuScreensCustom.cpp`, `src/core/Frontend.cpp` (`CUSTOM_FRONTEND_OPTIONS`) |
| HUD text, timers, messages | `CHud` (`src/renderer/Hud.h`), `CMessages` (`src/text/Messages.h`), `COnscreenTimer` (`src/control/OnscreenTimer.h`) |
| Radar blips | `CRadar` (`src/core/Radar.h`) |
| Checkpoint markers | `C3dMarkers` (`src/renderer/SpecialFX.h`), `CCoronas` (`src/renderer/Coronas.h`) |
| Time & weather | `CClock` (`src/core/Clock.h`), `CWeather` (`src/renderer/Weather.h`) |
| Feature toggles | `src/core/config.h` |

## Building

See the main [README](../../README.md) for the game. On Linux, CMake + conan
or premake (`premake5Linux`). Game-side multiplayer sources go under
`src/multiplayer/` (Plan 2).

The engine-free layer in `mp/` builds and tests on its own, with no game files:

```bash
cmake -S mp -B build-mp -G Ninja
cmake --build build-mp
ctest --test-dir build-mp --output-on-failure
```

It is also part of the top-level CMake build (option `REVC_MULTIPLAYER`, on by
default). The build produces:

| Target | What |
|---|---|
| `enet` | Vendored ENet 1.3.18 (`vendor/enet`, unmodified) |
| `mpcommon` | `mp/common`: protocol, byte streams, invite codes, session clock, pose buffer, `CNetClient` |
| `relaycore` | `CRelayServer` (`mp/relay/RelayServer.*`), shared by `revc-relay` and the tests |
| `revc-relay` | The invite-code relay server. See [relay-setup.md](relay-setup.md) |
| `revc-fakeclient` | Headless client for testing |
| `mp_tests` | doctest suites (`mp/tests`), including relay + client integration over loopback |

Try a session locally, with each command in its own terminal:

```bash
./build-mp/revc-relay --port 7777
```

```bash
./build-mp/revc-fakeclient --relay 127.0.0.1 --host --name Tommy
```

```bash
./build-mp/revc-fakeclient --relay 127.0.0.1 --join VICE-XXXX --name Lance --say hello
```

Tommy prints `[joined] 1 Lance` and `[chat] 1: hello`. Pressing Ctrl+C on Tommy
makes Lance print `[ended] host left` and exit non-zero.

### Network layer at a glance

- Every player connects only to the relay (ENet, 2 channels: 0 reliable, 1
  snapshots). Each packet is a 2-byte envelope `{route, player}` followed by a
  message whose first byte is its type. Uplink `player` is the target (for
  `MP_ROUTE_PLAYER`). Downlink `player` is the sender, or `MP_PLAYER_RELAY`.
- Types below 64 belong to the layer (relay control, chat, ping/pong). Types
  from `MSG_GAME_FIRST` (64) up are game messages: `CNetClient::SendGame`
  sends them and `INetClientListener::OnGameMessage` receives them unparsed.
- Session time is the host's clock. Joiners ping the host every second and
  `CSessionClock` keeps the lowest-RTT offset of the last 16 samples.

## Open questions

Track anything waiting on the owner here, and remove it once answered.

- GTA Vice City isn't installed on the dev machine yet. In-game testing waits
  for it. Until then, verify with unit tests and the headless fake client.
- The cloud relay's actual address/hostname, once the owner sets up the VPS.
