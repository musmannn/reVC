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
| [Testing checklist](testing.md) | In-game manual test checklist, to run once the game is installed. |

## Status

| Milestone | Scope | State |
|---|---|---|
| 1 | Invite-code sessions, free roam, races | **Code complete, in-game unverified.** Plan 1 is complete (network layer, relay, fake client). Plans 2 and 3 are written and build with `MULTIPLAYER` on Linux, macOS and Windows in CI. The engine-free race rules, route parser and race protocol are unit tested, and a whole race runs with fake clients. Still waiting on the game files: every check in [testing.md](testing.md), plus recording the 4 built-in race routes. |
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
| 2026-10-31 | `MULTIPLAYER` also needs `CUSTOM_FRONTEND_OPTIONS` (the menu page is a CFO page), and the build defines `NO_MULTIPLAYER` when `mp/` isn't part of it, so `REVC_MULTIPLAYER=OFF` builds plain single player. | Builder call |
| 2026-10-31 | Multiplayer menu text keys are at most 7 characters (`CMenuEntry::m_EntryName` is `char[8]`): `MP_RLAY` and `MP_LEAV` instead of the plan's `MP_RELAY`/`MP_LEAVE`, plus `MP_STAT` for the session row. | Builder call |
| 2026-10-31 | A session becoming ready starts the world through `DoSettingsBeforeStartingAGame` (the New Game path). When it ends, the frontend opens; the offline world stays loaded instead of a forced return to the start menu (VC has no in-game "quit to main menu"). | Builder call |
| 2026-10-31 | The network keeps servicing from `LoadingScreen` too, so long loads don't time the connection out; UI reactions wait for the next frame. | Builder call |
| 2026-10-31 | Remote players' seats follow their `PLAYER_STATE` (vehicle id + seat), so seating heals after lost messages and works for late joiners; `VEHICLE_SEAT` only carries the host's accept/reject and ownership changes. | Builder call |
| 2026-10-31 | The VC player can't normally get in as a passenger. Pressing enter at a car a friend is in warps you into a free passenger seat (`CPed::SetCarJack` hook), never dragging them out. | Builder call |
| 2026-10-31 | When a vehicle owner leaves, ownership only changes if the leaver owned it (`MpNextVehicleOwner`); a passenger leaving never takes the car away from its owner. | Builder call |
| 2026-10-31 | Remote shots draw a tracer along the shooter's camera direction (`CNetCombat::OnLocalShot`), for instant-hit weapons only; projectiles show through their anims. | Builder call |
| 2026-10-31 | The multiplayer respawn is its own branch in `CGameLogic::Update` (no money/weapon loss, no `PassTime`, no taxi) instead of `CRestart::OverrideNextRestart`; `CNetCombat::SetRespawnOverride` lets races respawn at the last checkpoint. | Builder call |
| 2026-10-31 | Chat and free-roam menu disable player controls with new bits `0x200` and `0x400` (the engine uses up to `0x100`). Cheats are ignored while typing in chat or a menu field (`CPad::AddToPCCheatString`). | Builder call |
| 2026-10-31 | Saving is blocked in a session (the save point shows "Saving is disabled in multiplayer"); loading a save leaves the session. | Builder call |
| 2026-10-31 | `'~'` is dropped from names and chat: the game's font treats it as the start of a colour token. | Builder call |
| 2026-10-31 | Racers advance to the next checkpoint locally the moment they cross it; the host's standings are the truth for positions and results. | Builder call |
| 2026-10-31 | The host computes each racer's distance to their next checkpoint from the `PLAYER_STATE` positions it already gets, so racers don't send it. | Builder call |
| 2026-10-31 | At race start every racer, passengers included, is warped out of their car into a fresh race car on the grid. | Builder call |
| 2026-10-31 | Race routes are text `.route` files in `data/mp/races/`, listed in `data/mp/races/index.txt`: `CFileMgr` can't list a directory, so the index stands in for the plan's `*.route` glob. `/rec save` adds new routes to it. | Builder call |
| 2026-10-31 | The host side of a race (invite, accepts, start, standings, end) is the engine-free `CRaceHost` in `mp/common`, shared by the game host and the fake client's `--start-race`, and unit tested. Late joiners get the open invite. | Builder call |
| 2026-10-31 | Race checkpoint cylinders are drawn at the trigger radius (`C3dMarkers` size = radius, like the script's spheres), not twice it, so what you see is what counts. | Builder call |
| 2026-10-31 | The race freeze uses its own player-control bit `0x800` instead of `PLAYERCONTROL_CUTSCENE`, so it can't fight cutscenes. | Builder call |
| 2026-10-31 | Catch-up for late joiners (our vehicles, time/weather, an open race invite) goes out on their first `PLAYER_STATE`, not on the join notice: game messages sent while they're still loading the world are dropped. | Builder call |
| 2026-10-31 | Added `mp/common/NetInit.*` (ref-counted `enet_initialize`), which the relay and `CNetClient` share. This file isn't in the plan's file list. | Builder call |
| 2026-10-31 | On `Stop()` the relay drains for 250 ms and then destroys the host without forcing disconnects. ENet discards a peer's undelivered packets when a disconnect arrives, which would swallow `SESSION_ENDED`. | Builder call |
| 2026-10-31 | Received chat text and names are filtered to printable ASCII in `CNetClient` before they reach the game. | Builder call |
| 2026-10-31 | `REVC_MULTIPLAYER` defaults to OFF on Switch, PS2 and Android (they're out of scope, and the Switch CI builds the top-level CMake). It defaults to ON elsewhere, as the plan says. | Builder call |
| 2026-10-31 | `vendor/enet` also keeps ENet's `README` and `ChangeLog`, and `vendor/doctest` keeps `LICENSE.txt`, next to the plan's files. All are unmodified copies of the tags. | Builder call |
| 2026-10-31 | `CNetClient` detects a re-entrant `Host()`/`Join()` from inside a callback with a generation counter. Comparing `ENetHost` pointers failed on macOS, where malloc reuses the address. | Builder call |
| 2026-10-31 | Relay hardening beyond the plan: ENet `maximumPacketSize` 2 KB and `maximumWaitingData` 64 KB (the defaults allow 32 MB per peer), at most 16 connections per IP (`SRelayConfig::maxPeersPerAddress`, an added field), and at most 4096 events per `Service()`. Known gap: a member that is slow to acknowledge can make the relay queue reliable traffic for it until its ENet timeout (≤ 15 s). | Builder call |

## Race routes

`.route` files live in `gamefiles/data/mp/races/` (copied into the game's
`data/mp/races/`) and are listed in `index.txt` there. They're plain text with
`#` comments and one directive per line:

```
name Ocean Drive Sprint
type sprint            # or circuit
laps 1                 # circuits: 1..9; sprints are always 1
vehicle 141            # model id (141 = Infernus)
grid  X Y Z HEADING    # 1..8 slots, in grid order; heading in degrees
cp    X Y Z RADIUS     # 2..64 checkpoints in driving order; the last one is the finish
```

A bad file is skipped with a reason in the debug log, and the rest still load.
Record routes in a session with the chat commands `/rec new NAME`,
`/rec type sprint|circuit`, `/rec laps N`, `/rec vehicle`, `/rec grid`,
`/rec cp [radius]`, `/rec undo` and `/rec save`. `/pos` prints your coordinates.

To try a race without the game: start a relay, then
`revc-fakeclient --relay 127.0.0.1 --host --start-race "Test"` and two
`revc-fakeclient --relay 127.0.0.1 --join VICE-XXXX --bot race`.

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
| **Multiplayer game code** | `src/multiplayer/`: `CNetGame` (session, dispatch), `CRemotePlayer`, `CNetVehicles`, `CNetCombat`, `CNetChat`, `CFreeroamMenu`, `MpMenu` (frontend page), `MpText`, `CSpawnPoints` |
| Multiplayer engine hooks | `CGame::Process/Initialise/ReInitGameObjectVariables/ShutDown*` (`Game.cpp`), `FrontendIdle`/`LoadingScreen`/`Render2dStuff` (`main.cpp`), `CTheScripts::Init/OpenScript` (`Script.cpp`), `CText::Get`, `CMenuManager::Process` + save menu (`Frontend.cpp`), `CPed::InflictDamage`, `CPed::SetCarJack`, `CWeapon::Fire`, `CGameLogic::Update` (WASTED), `CPad::AddToPCCheatString`, INI load/save (`re3.cpp`). All inside `#ifdef MULTIPLAYER`. |

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

- The default relay is `MP_DEFAULT_RELAY` = `127.0.0.1:7777` in `src/multiplayer/NetGame.h` until the cloud VPS exists. It's a one-line change.
- The 4 built-in race routes (Ocean Drive Sprint, Starfish Loop, Bridge Run, Little Havana Circuit) aren't recorded yet. Record them in-game with `/rec` and list them in `gamefiles/data/mp/races/index.txt`.
- Placeholders that need the game to check: the spawn points (`src/multiplayer/SpawnPoints.cpp`), the default remote-player skins (`sDefaultSkins` in `RemotePlayer.cpp`) and the fake client's `--bot drive` model (132).

- GTA Vice City isn't installed on the dev machine yet. In-game testing waits
  for it. Until then, verify with unit tests and the headless fake client.
- The cloud relay's actual address/hostname, once the owner sets up the VPS.
