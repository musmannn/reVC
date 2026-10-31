# Multiplayer in-game test checklist

These checks need the original GTA Vice City files, so CI can't run them.
Run them once the game is installed and record the result and date next to
each item (`✅ 2026-11-05`, or `❌` plus a note).

The engine-free layer (protocol, relay, client, race rules) is covered by
`ctest --test-dir build-mp`. This file is for everything that needs the game
running.

## Setup: two instances on one PC

1. Build `revc-relay` and `revc-fakeclient` (see `README.md` in this folder) and
   start a relay: `./build-mp/revc-relay --port 7777`.
2. Make a second copy of the game directory (e.g. `GTAVC` → `GTAVC-2`) with the
   same reVC binary, so the two instances don't share `reVC.ini`.
3. In each copy, set *Multiplayer → Relay server* to `127.0.0.1:7777` and give
   each a different *Name*.
4. Start both. Host in one, join with the code in the other. Run them windowed.
5. A third "player" is a bot:
   `./build-mp/revc-fakeclient --relay 127.0.0.1 --join VICE-XXXX --name Bot --bot walk --at X,Y,Z`.

Simulated latency on Linux (remove afterwards with `sudo tc qdisc del dev lo root`):

```bash
sudo tc qdisc add dev lo root netem delay 75ms
```

## Plan 2, Task 3: session lifecycle and world start

- [ ] The host gets a `VICE-XXXX` code and spawns in Ocean Beach with a bat and a pistol.
- [ ] `revc-fakeclient --join CODE --name Lance` makes Lance appear in the session.
- [ ] Pause for 30 s: the fake client stays connected and nobody times out.
- [ ] Killing the relay shows "Connection lost" and opens the frontend menu.
- [ ] With no session, New Game still runs the story normally (main.scm, intro).
- [ ] Holding `R`/`D` at New Game still picks the debug scripts as before.
- [ ] Spawn point coordinates in `src/multiplayer/SpawnPoints.cpp` are on the ground near the Ocean View Hotel (they're placeholders).

## Plan 2, Task 4: Multiplayer frontend page

- [ ] *Multiplayer* appears on the start menu and the pause menu.
- [ ] Type a name with Backspace; Esc restores the old value; the name persists after restarting the game (`reVC.ini` `[Multiplayer] Name`).
- [ ] While typing, arrow keys/Enter/Esc don't navigate the menu, and Esc doesn't close it.
- [ ] Shift gives capitals in the name; the code field is always upper case.
- [ ] Join with the wrong code `VICE-ZZZZ` shows "No session with that code" in the *Session* row.
- [ ] The 9th player gets "Session is full (8 players)".
- [ ] In a session, the *Session* row shows `VICE-XXXX - n/8 players`; Name/Relay/Host/Join are greyed out and Leave works.

## Plan 2, Task 5: remote players on foot

- [ ] `revc-fakeclient --join CODE --bot walk --at <spawn x,y,z>`: a ped walks a smooth 10 m circle with a name tag, a health bar and a coloured radar blip.
- [ ] Two real instances: walking, running, sprinting, jumping and crouching look right on the other screen.
- [ ] A friend teleports (e.g. with the free-roam menu or a respawn) and their ped jumps there instead of sliding across the map.
- [ ] With 150 ms simulated latency (`tc netem delay 75ms`), movement has no visible stutter.
- [ ] A friend's weapon in hand matches what they hold.
- [ ] Remote ped models (`sDefaultSkins` in `src/multiplayer/RemotePlayer.cpp`) are sensible civilians (placeholder ids).
- [ ] Peds/traffic AI ignore remote players (no fleeing or fighting), and remote players are never despawned when far away.
