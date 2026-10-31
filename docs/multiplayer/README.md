# Multiplayer: project context for contributors and agents

Read this first if you're working on reVC multiplayer. It's the single place
for where things stand, what's been decided, and where the relevant code is.
Keep it up to date as work lands.

## Documents

| Doc | What it is |
|---|---|
| [Design spec](../superpowers/specs/2026-10-31-multiplayer-design.md) | What we're building and why: goals, non-goals, anti-goals, constraints. The source of truth for scope. |
| Implementation plan | *Not written yet.* Will live in `docs/superpowers/plans/` once the design is approved. |

## Status

| Milestone | Scope | State |
|---|---|---|
| 1 | Invite-code sessions, free roam, races | Design approved; plan in progress |
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
| 2026-10-31 | Free roam gives a **spawn loadout + weapon/health/armour menu**; no world pickups or Ammu-Nation in milestone 1. | Owner |
| 2026-10-31 | Race results ordered by racers' own crossing timestamps on a shared session clock, not by packet arrival. | Builder call |
| 2026-10-31 | Up to 8 players per session. Story script does not run in multiplayer. | Builder call |
| 2026-10-31 | Remote players are extra ped entities fed by the network. The local player stays the only `CPlayerPed`. | Builder call |

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

See the main [README](../../README.md). On Linux, CMake + conan or premake
(`premake5Linux`). New multiplayer sources go under `src/multiplayer/` (game
side) and `tools/relay/` or similar (lobby/relay server). The plan will fix the
exact layout.

## Open questions

Track anything waiting on the owner here, and remove it once answered.

- GTA Vice City isn't installed on the dev machine yet. In-game testing waits
  for it. Until then, verify with unit tests and the headless fake client.
- The cloud relay's actual address/hostname, once the owner sets up the VPS.
