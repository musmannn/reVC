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

## Plan 2, Task 6: vehicles

- [ ] `revc-fakeclient --join CODE --bot drive --at <x,y,z>`: a car drives a smooth 40 m circle with the bot's ped at the wheel (`--model` default 132 is a placeholder).
- [ ] Two instances: hijack a taxi; the friend sees it appear and drive smoothly.
- [ ] Press enter next to a friend's car: you warp in as a passenger (you never drag them out); both views match while they drive.
- [ ] As the passenger, your controls don't steer the friend's car.
- [ ] Both players try the driver seat of an empty networked car at once: one gets it, the other is put back outside.
- [ ] Quit the game while driving with a passenger: the passenger keeps the car (it becomes theirs/local) and doesn't fall through the map.
- [ ] A friend leaves while their parked car is nearby: it disappears unless someone is in it.
- [ ] Drive at 150 ms latency: no stutter; driving into the friend's car pushes your car.
- [ ] A friend spawns a vehicle far away / a model you haven't loaded (e.g. a Hunter): it appears once streamed, without a crash.
- [ ] A friend's car blows up on your screen when it blows up on theirs.

## Plan 2, Task 7: combat, death and respawn

- [ ] Shoot a friend: their health drops on their HUD (not just on yours).
- [ ] Armour absorbs damage first.
- [ ] A kill shows "Tommy killed Lance" on both screens; dying alone shows "Lance died".
- [ ] The victim respawns at a spawn point with a bat and pistol, no money lost, clock unchanged, no hospital taxi.
- [ ] The friend's shots show tracers and gunshot sounds on your screen, but never hurt anything there.
- [ ] Running a friend over in a car damages them (on their screen); their car hitting you doesn't double the damage.
- [ ] Dying in a car respawns you on foot; the car stays behind.
- [ ] Both players die at the same time: both respawn normally.
- [ ] Single player: dying still costs $100 and weapons, passes time, and takes you to hospital.

## Plan 2, Task 8: chat, notices, time and weather

- [ ] `T` opens chat; messages go both ways and show with coloured names, top left, for 10 s (all lines while typing).
- [ ] While typing, `W`/`F`/arrow keys don't move or exit the car, and typing a cheat (e.g. `THUGSTOOLS`) does nothing.
- [ ] `Esc` cancels chat without opening the pause menu; `Enter` on an empty line just closes it.
- [ ] Pressing `Enter` to send doesn't also make you get out of the car.
- [ ] "Lance joined" / "Lance left" notices appear.
- [ ] The host changes the weather (debug menu) and the friend sees it within 10 s; game time matches within 2 minutes.
- [ ] After leaving the session the weather goes back to normal.

## Plan 2, Task 9: free-roam menu

- [ ] `F2` opens the menu; arrows/Enter/Backspace navigate; `F2` or *Close* closes it; player controls are off while it's open.
- [ ] Every vehicle entry spawns the right model 5 m ahead, with you in the driver seat, and the friend sees it.
- [ ] Spawning a second car removes the first if nobody is in it.
- [ ] Every weapon category gives the weapon (500 ammo, melee 1) and puts it in hand.
- [ ] *Health* and *Armour* fill up.
- [ ] `F2` while typing chat does nothing; `T` while the menu is open does nothing.
- [ ] Spawning a boat on land shows "Must be on water".

## Plan 3, Task 4: in-game races

Engine-free race rules and the race protocol are unit tested (`ctest`), and a
whole race runs without the game: relay + `revc-fakeclient --host --start-race "Test"` + two
`revc-fakeclient --join CODE --bot race`. These checks need the game:

- [ ] F2 → *Start race >* lists the routes from `data/mp/races/index.txt`; picking one posts "Tommy started … press F3 to join (15s)" to everyone, with a countdown top right.
- [ ] A full race against 2 `--bot race` bots: grid, frozen 3-2-1-GO with beeps, checkpoints, results.
- [ ] Two real players race; both see identical final results.
- [ ] Checkpoints: yellow cylinder + radar blip for the current one, a fainter one for the next, red for the finish; cutting a corner past a checkpoint doesn't count it.
- [ ] HUD shows POS n/m, LAP (circuits), TIME and the checkpoint counter.
- [ ] Blow up the car mid-race: respawn at the last checkpoint in a fresh car, facing the next one, frozen for 1 s; the timer keeps running.
- [ ] Hold `H` for 1.5 s while flipped: respawn the same way. Leave the car for 5 s: respawn too.
- [ ] Die mid-race: respawn at the last checkpoint (not a free-roam spawn point), then get a fresh car.
- [ ] Kill a racer's game mid-race: they're DNF and the race still ends.
- [ ] Nobody accepts: only the starter races (a time trial).
- [ ] Start a race while one is running: "A race is already running".
- [ ] Non-racers see the invite and "Lance won … in mm:ss.hh" only.
