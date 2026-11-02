# Installing reVC and playing multiplayer

This guide covers Windows, Linux and macOS: installing reVC with the installer
(or by hand), running a multiplayer server, hosting a game and joining one.

- [1. What you need](#1-what-you-need)
- [2. Quick start](#2-quick-start)
- [3. Windows](#3-windows)
- [4. Linux](#4-linux)
- [5. macOS](#5-macos)
- [6. The installer in detail](#6-the-installer-in-detail)
- [7. Installing by hand (what to copy where)](#7-installing-by-hand-what-to-copy-where)
- [8. How multiplayer works](#8-how-multiplayer-works)
- [9. Hosting a relay](#9-hosting-a-relay) (same PC, home network, home PC on the internet, cloud server)
- [10. Hosting and joining a game](#10-hosting-and-joining-a-game)
- [11. Updating and uninstalling](#11-updating-and-uninstalling)
- [12. Troubleshooting](#12-troubleshooting)

## 1. What you need

- **Your own copy of GTA Vice City for PC** (Steam, retail or similar). reVC
  contains no Rockstar files. It runs on top of your game files.
- **Python 3.7 or newer**, to run the installer.
- About **5 GB of free space** if the installer makes a copy of the game, or
  about 1 GB if it installs into your game folder.
- An internet connection: the installer downloads the code and build tools.

You don't need to know how to compile anything. The installer builds reVC for
your computer, which takes 5 to 15 minutes the first time.

## 2. Quick start

1. Install Python and the build tools for your system:
   [Windows](#3-windows), [Linux](#4-linux) or [macOS](#5-macos). On Windows
   and Linux the installer can install the build tools for you.
2. Get the installer. Either clone the repository:

   ```bash
   git clone --recursive -b multiplayer https://github.com/musmannn/reVC.git
   ```

   or download only
   [`installer/revc_installer.py`](https://github.com/musmannn/reVC/raw/multiplayer/installer/revc_installer.py).
   On its own, it downloads the rest of the code for you.
3. Run it:
   - **Windows:** double-click `installer\install-gui.bat`, or run
     `py installer\revc_installer.py`.
   - **Linux:** `python3 installer/revc_installer.py`
   - **macOS:** double-click `installer/install-gui.command`, or run
     `python3 installer/revc_installer.py`.
4. In the window:
   - **reVC source code**: leave it as it is. If the folder isn't a reVC
     checkout yet, the code is downloaded there.
   - **GTA Vice City folder**: click **Find it for me** (it searches your
     Downloads folder and Steam) or **Browse…** to pick the folder that has
     `data`, `models` and `Audio` inside.
   - **Put reVC**: *Into the game folder* puts everything in your game folder.
     *Into a new folder* copies the game somewhere else first and leaves your
     original untouched.
   - Click **Install** and wait for "All done!".
5. Click **Play**. To play online, read [How multiplayer works](#8-how-multiplayer-works).

## 3. Windows

### Prerequisites

1. **Python 3** from [python.org/downloads](https://www.python.org/downloads/).
   In the setup, tick **Add python.exe to PATH**. (Python from the Microsoft
   Store works too.)
2. **Visual Studio with C++.** With **Install missing tools** ticked (the
   default), the installer installs *Visual Studio Build Tools 2022* with the
   C++ workload through `winget`. That is a few GB, and Windows asks for
   permission. Or install it yourself first: [Visual Studio
   Community](https://visualstudio.microsoft.com/vs/community/) with the
   **Desktop development with C++** workload. Visual Studio 2019, 2022 and
   2026 all work.
3. **Git.** The installer installs it through `winget` too, or get it from
   [git-scm.com](https://git-scm.com/download/win).

`winget` comes with Windows 10 (21H1 and later) and Windows 11. If you don't
have it, install Visual Studio and Git by hand as above.

### Install

Double-click `installer\install-gui.bat` (or run `py installer\revc_installer.py`
in a terminal) and follow [the quick start](#2-quick-start).

- **Renderer**: `d3d9` (Direct3D 9) suits most PCs. If you get graphics
  problems, run the installer again with `gl3` (OpenGL).
- Installing **into the game folder** under `C:\Program Files (x86)` needs
  administrator rights. Pick **Into a new folder** instead, e.g. the default
  `%USERPROFILE%\Games\reVC-Multiplayer`, or run the installer as administrator.
- The installer adds a **reVC Multiplayer** shortcut to your desktop.

The first time you start `reVC.exe`, SmartScreen may warn about it. Click
**More info → Run anyway**. The first time you run the relay, Windows
Defender Firewall asks whether to allow it. Allow it on private networks
(and public ones if you're hosting for the internet).

The older PowerShell installer still works:
`installer\install-windows.bat [-Source DIR] [-Dest DIR] [-Renderer d3d9|gl3] [-SkipBuild]`.

## 4. Linux

### Prerequisites

You need Python 3 with Tkinter for the window. Without Tkinter, or without a
display (e.g. over SSH), the installer runs in the terminal instead.

| Distribution | Install Python and Tkinter |
|---|---|
| Debian, Ubuntu, Mint, Pop!_OS | `sudo apt install python3 python3-tk` |
| Fedora | `sudo dnf install python3 python3-tkinter` |
| Arch, Manjaro | `sudo pacman -S python tk` |
| openSUSE | `sudo zypper install python3 python3-tk` |

With **Install missing packages** ticked, the installer installs the build
tools and libraries itself and asks for your password (through `pkexec` in the
window, `sudo` in the terminal). To install them yourself instead:

| Distribution | Build tools and libraries |
|---|---|
| Debian, Ubuntu | `sudo apt install build-essential cmake git pkg-config libglfw3-dev libopenal-dev libmpg123-dev libgl-dev libx11-dev` |
| Fedora | `sudo dnf install gcc-c++ make cmake git pkgconf-pkg-config glfw-devel openal-soft-devel mpg123-devel mesa-libGL-devel libX11-devel` |
| Arch | `sudo pacman -S --needed base-devel cmake git pkgconf glfw openal mpg123 mesa libx11` |
| openSUSE | `sudo zypper install gcc-c++ make cmake git pkg-config libglfw-devel openal-soft-devel mpg123-devel Mesa-libGL-devel libX11-devel` |

### Install

```bash
python3 installer/revc_installer.py
```

Then follow [the quick start](#2-quick-start). If your game came from Steam,
**Find it for me** looks in `~/.steam/steam/steamapps/common` too. Linux
works with the Windows game files.

The installer adds **reVC Multiplayer** to your applications menu. To start it
from a terminal, run it from the install folder:

```bash
cd ~/Games/reVC-Multiplayer && ./reVC
```

## 5. macOS

### Prerequisites

1. **Xcode Command Line Tools:** run `xcode-select --install` in Terminal and
   click Install.
2. **Homebrew** from [brew.sh](https://brew.sh). On Apple Silicon, follow the
   "Next steps" it prints so `brew` is on your PATH.
3. **Python with Tkinter:** `brew install python python-tk`.

The installer installs the rest through Homebrew (`cmake`, `git`, `glfw`,
`openal-soft`, `mpg123`).

### Install

Double-click `installer/install-gui.command` (the first time, right-click →
**Open**), or run:

```bash
python3 installer/revc_installer.py
```

Start the game by double-clicking `reVC.app` in the install folder. Keep it
there: reVC finds the game files next to the app. If macOS says the app
"can't be opened", right-click it → **Open** once, or run
`xattr -dr com.apple.quarantine reVC.app`. Game files from a Windows
install work on macOS as they are.

The older shell installer still works:
`./installer/install-macos.sh [--source DIR] [--dest DIR] [--skip-build]`.

## 6. The installer in detail

`installer/revc_installer.py` uses only Python's standard library. It does
this, in order:

1. **Checks your game folder.** It must contain `data/gta_vc.dat` and
   `models/gta3.img`.
2. **Checks the build tools** and installs missing ones if you let it.
3. **Gets the code.** If the source folder isn't a reVC checkout, it runs
   `git clone -b multiplayer`. With **Update to the latest code** it runs
   `git pull`. Then it fetches the `vendor/librw` submodule.
4. **Builds** reVC, `revc-relay` and `revc-fakeclient`:
   - Linux and macOS: CMake, into `build/linux` or `build/macos`.
   - Windows: premake and MSBuild for the game (into `bin\`), CMake for the
     tools (into `build\windows\mp`).
5. **Copies the game** into the new folder, if you chose one. If the folder
   already has the game, it skips this, so re-running is quick.
6. **Backs up** every original file that reVC replaces into `reVC-backup`
   (only the first time), then copies `gamefiles/` over the game.
7. **Copies the programs**: `reVC` / `reVC.exe` / `reVC.app` (plus
   `OpenAL32.dll` and `libmpg123-0.dll` on Windows), `revc-relay`,
   `revc-fakeclient`, this guide, and a `start-relay` script.
8. **Adds a shortcut**: desktop on Windows, applications menu on Linux.

When it's done, three buttons appear: **Play** starts the game, **Start relay**
runs a relay inside the installer window (its log shows there; click again
to stop it) and **Open folder** opens the install folder.

### Terminal mode

Add `--cli` to run without a window, e.g. on a server or over SSH:

```bash
python3 installer/revc_installer.py --cli --game "/path/to/GTA Vice City"
```

| Option | What it does |
|---|---|
| `--cli` | Run in the terminal instead of opening a window. |
| `--game DIR` | Your GTA Vice City folder. Default: the first one found in Downloads or Steam. |
| `--dest DIR` | Install into a copy of the game in `DIR` instead of into the game folder itself. |
| `--repo DIR` | Where the reVC code is, or where to download it. Default: the checkout the installer is in, else `~/reVC`. |
| `--relay-only` | Build and install only `revc-relay` and `revc-fakeclient`. No game needed. Goes to `~/Games/reVC-Relay` unless you give `--dest`. |
| `--renderer d3d9\|gl3` | Windows only. Direct3D 9 (default) or OpenGL. |
| `--update` | `git pull` before building. |
| `--skip-build` | Reuse the last build. Handy for installing again to another folder. |
| `--no-deps` | Don't install missing tools or libraries; say what's missing and stop. |
| `--no-shortcut` | Don't add a shortcut. |

All of these work with the window too, to fill in its fields.

## 7. Installing by hand (what to copy where)

If you'd rather not use the installer, build reVC yourself and copy these
files into your game folder (or a copy of it):

| Copy this | Into the game folder |
|---|---|
| everything inside `gamefiles/` (`data/`, `models/`, `TEXT/`, `neo/`, `gamecontrollerdb.txt`) | merged over the game's own folders, replacing files |
| the game: `build/linux/src/reVC`, `build/macos/src/reVC.app` or `bin\win-amd64-librw_d3d9-oal\Release\reVC.exe` | next to `gta-vc.exe` |
| Windows only: `vendor\openal-soft\dist\Win64\OpenAL32.dll` and `vendor\mpg123\dist\Win64\libmpg123-0.dll` | next to `reVC.exe` |
| `revc-relay` and `revc-fakeclient` (`build/<os>/mp/`, or `build\windows\mp\Release\` on Windows) | anywhere; next to the game is handy |

Back up the files `gamefiles/` replaces first if you want to undo it later.
Always run reVC from the game folder, so it finds the game files.

**Linux and macOS build** (after the [prerequisites](#4-linux)):

```bash
cmake -S . -B build/linux -DCMAKE_BUILD_TYPE=Release -DREVC_VENDORED_LIBRW=ON -DLIBRW_PLATFORM=GL3 -DLIBRW_GL3_GFXLIB=GLFW -DREVC_AUDIO=OAL
```

```bash
cmake --build build/linux --parallel
```

On Linux, `./build.sh "/path/to/GTA Vice City"` does the build and the copying
in one go.

**Windows build** (in a *Developer Command Prompt for VS*, from the repo folder):

```bash
premake5.exe vs2022 --with-librw --no-full-paths
```

```bash
msbuild -m build\reVC.sln /p:Configuration=Release /p:Platform=win-amd64-librw_d3d9-oal
```

```bash
cmake -S mp -B build\windows\mp && cmake --build build\windows\mp --config Release --target revc-relay revc-fakeclient
```

Use `vs2019` or `vs2026` to match your Visual Studio.

## 8. How multiplayer works

Every player connects to one small server program, the **relay**
(`revc-relay`). The relay hands out invite codes like `VICE-K7QM` and passes
game traffic between the players in a session. Because everyone connects *to*
the relay, players never forward ports or share IP addresses. Only the relay
must be reachable.

```
  Player A (host) ─┐
  Player B ────────┼──▶  revc-relay  (UDP port 7777)
  Player C ────────┘
```

So for any game, someone first runs a relay, then:

- one player **hosts** a session and gets an invite code,
- everyone else **joins** with that code,
- everyone uses the **same relay address** in the game.

Up to 8 players per session. The relay has no game logic and uses very little
CPU or bandwidth, so any old PC or the smallest cloud server is enough.

## 9. Hosting a relay

Pick the option that fits who you're playing with.

| Who's playing | Where the relay runs | Address players type in the game |
|---|---|---|
| Just you, testing | your PC | `127.0.0.1:7777` |
| Friends on the same Wi-Fi/LAN | any PC on that network | that PC's LAN address, e.g. `192.168.1.20:7777` |
| Friends over the internet | your home PC with a forwarded port | your public IP, e.g. `203.0.113.5:7777` |
| Friends over the internet, always on | a cloud server (VPS) | the server's public IP or domain |

The relay listens on **UDP port 7777**. Options:

```
revc-relay [--port N] [--bind ADDR] [--max-sessions N]
```

### On your own PC (testing, LAN)

Use any of these:

- In the installer, click **Start relay** after installing.
- Double-click `start-relay.bat` (Windows) or `start-relay.command` (macOS)
  in the install folder, or run `./start-relay.sh` (Linux).
- Or in a terminal, from the install folder: `./revc-relay --port 7777`
  (`revc-relay.exe --port 7777` on Windows).

Leave it running while you play. For LAN play, other players type your PC's
LAN address. Find it with `ipconfig` (Windows, "IPv4 Address"),
`ip -4 addr` (Linux) or *System Settings → Network* (macOS). Allow the relay
through your firewall when asked.

### On your home PC, over the internet

Do the above, plus:

1. **Forward UDP port 7777** on your router to your PC's LAN address. The
   setting is usually under *Port Forwarding*, *Virtual Server* or *NAT*.
   Give your PC a fixed LAN address (a *DHCP reservation*) so the rule keeps
   working.
2. **Allow it through the firewall:**
   - Windows (in an administrator terminal):
     ```bash
     netsh advfirewall firewall add rule name="reVC relay" dir=in action=allow protocol=UDP localport=7777
     ```
   - Linux: `sudo ufw allow 7777/udp`, or
     `sudo firewall-cmd --permanent --add-port=7777/udp && sudo firewall-cmd --reload`
   - macOS: accept the "allow incoming connections" prompt.
3. **Find your public IP** (search "what is my IP", or
   `curl https://api.ipify.org`) and give it to your friends. Home IPs can
   change, so a free dynamic-DNS name (e.g. DuckDNS) helps.
4. On your own PC, keep using `127.0.0.1:7777`. Many routers can't connect to
   their own public IP from inside.

### On a cloud server (VPS)

Any small Linux server works: the Oracle Cloud *Always Free* tier, or a ~$5/month
VPS from any provider. Pick a region close to the players. The server needs no
game files.

1. Connect with SSH and install the relay with the installer in terminal mode
   (on Debian/Ubuntu; it installs `git`, `cmake` and a compiler with `sudo`):

   ```bash
   sudo apt update && sudo apt install -y python3 git
   ```

   ```bash
   curl -LO https://github.com/musmannn/reVC/raw/multiplayer/installer/revc_installer.py
   ```

   ```bash
   python3 revc_installer.py --cli --relay-only
   ```

   This downloads the code to `~/reVC` and puts the relay in `~/Games/reVC-Relay`.
2. Install it as a service, so it starts on boot and restarts if it stops:

   ```bash
   sudo install -m 755 ~/Games/reVC-Relay/revc-relay /usr/local/bin/revc-relay
   ```

   ```bash
   sudo useradd --system --no-create-home --shell /usr/sbin/nologin revc
   ```

   Create `/etc/systemd/system/revc-relay.service` (e.g. with
   `sudo nano /etc/systemd/system/revc-relay.service`):

   ```ini
   [Unit]
   Description=reVC multiplayer relay
   After=network-online.target
   Wants=network-online.target

   [Service]
   User=revc
   ExecStart=/usr/local/bin/revc-relay --port 7777
   Restart=always
   RestartSec=2
   NoNewPrivileges=true
   ProtectSystem=strict
   ProtectHome=true
   PrivateTmp=true

   [Install]
   WantedBy=multi-user.target
   ```

   ```bash
   sudo systemctl daemon-reload && sudo systemctl enable --now revc-relay
   ```

   Watch its log with `journalctl -u revc-relay -f`.
3. **Open UDP 7777 in both firewalls.** Most "it doesn't connect" problems are
   one of these.
   - **The provider's firewall** (called a security list, security group or
     firewall in the web console): allow inbound **UDP 7777** from
     `0.0.0.0/0`. On Oracle Cloud: *Networking → Virtual Cloud Networks → your
     VCN → Security Lists → Default → Add Ingress Rule*.
   - **The server's own firewall:** `sudo ufw allow 7777/udp`. Oracle's
     Ubuntu images also block everything in iptables:
     ```bash
     sudo iptables -I INPUT -p udp --dport 7777 -j ACCEPT && sudo netfilter-persistent save
     ```
4. **Test it from your own PC**, from the install folder:

   ```bash
   ./revc-fakeclient --relay <server-ip> --host --name Test
   ```

   It should print `[ready] code=VICE-XXXX` within a second. `[failed] not found`
   after 5 seconds means the relay can't be reached: check both firewalls.

To update the relay later, run the installer again with `--update`, then
repeat the `sudo install` line and `sudo systemctl restart revc-relay`.
[docs/multiplayer/relay-setup.md](docs/multiplayer/relay-setup.md) has more
detail.

## 10. Hosting and joining a game

### Host

1. Make sure a relay is running (see [Hosting a relay](#9-hosting-a-relay)).
2. Start reVC and choose **Multiplayer** on the main menu (it's on the pause
   menu too).
3. Set **Name** to your name, and **Relay server** to the relay's address,
   e.g. `127.0.0.1:7777` or `203.0.113.5:7777`. The port can be left off if
   it's 7777.
4. Choose **Host Session**. You get an invite code like `VICE-K7QM`. Send it
   to your friends, together with the relay address.

### Join

1. Start reVC and choose **Multiplayer**.
2. Set **Name**, and **Relay server** to the **same relay** the host uses.
3. Type the invite code under **Code** and choose **Join with Code**.

Everyone in a session must use the same relay and the same version of reVC.

### In a session

| Key | Action |
|---|---|
| `T` | Chat |
| `F2` | Free-roam menu: cars, weapons, health, armour, start a race or deathmatch |
| `F3` | Join a race or match someone started |
| Hold `H` | Respawn your race car |

Single player works as normal: **New Game** on the main menu.

### No one to play with?

Add a bot that walks around next to you. Host a session, type `/pos` in chat
to see your position, then from the install folder:

```bash
./revc-fakeclient --relay 127.0.0.1 --join VICE-XXXX --bot walk --at X,Y,Z
```

## 11. Updating and uninstalling

**Update:** run the installer again with **Update to the latest code**
ticked (`--update` in terminal mode). It pulls the code, rebuilds and
refreshes the reVC files, but doesn't copy the game again. Saves and settings
are kept. Everyone you play with needs the same version.

**Uninstall:**

- If you installed into **a new folder**, delete that folder.
- If you installed **into the game folder**, delete `reVC` / `reVC.exe` /
  `reVC.app`, `revc-relay`, `revc-fakeclient`, the `start-relay` script,
  `INSTALLATION.md`, and on Windows `OpenAL32.dll` and `libmpg123-0.dll`. Then
  copy the contents of `reVC-backup` back over the game folder and delete
  `reVC-backup`. Files reVC added (like `data/freeroam_miami.scm`,
  `data/mp/` and `neo/`) do no harm, and the original game ignores them.
- Remove the shortcut: `reVC Multiplayer` on the Windows desktop, or
  `~/.local/share/applications/revc-multiplayer.desktop` on Linux.
- The code and builds are in the source folder (e.g. `~/reVC`). Delete it if
  you don't want to rebuild later.

## 12. Troubleshooting

| Problem | Fix |
|---|---|
| "doesn't look like GTA Vice City" | Pick the folder that directly contains `data` and `models`, not a folder above it. |
| The window doesn't open, it runs in the terminal | Tkinter is missing. Install it (see [Linux](#4-linux) or [macOS](#5-macos)), or use terminal mode. |
| "vendor/librw is empty" | The code came from a ZIP. Use Git, or let the installer download it to an empty folder. |
| "isn't empty and isn't a reVC checkout" | The source folder has other files. Pick an empty folder, or your reVC checkout. |
| "has no multiplayer code" | Your checkout is on another branch: `git checkout multiplayer`. |
| Linux: package install fails or asks nothing | Install the packages yourself with the commands in [Linux](#4-linux), then run the installer with `--no-deps`. |
| Windows: "Visual Studio with C++ is needed" | Install Visual Studio Community with **Desktop development with C++**, or let the installer do it (needs `winget`). |
| Windows: "Visual Studio is installed without C++" | Visual Studio Installer → Modify → tick **Desktop development with C++**. |
| "no permission to write" | The game is in a protected folder like Program Files. Choose **Into a new folder**, or run the installer as administrator. |
| The build fails | Scroll up in the log for the first `error:` line. Run the installer again with **Update to the latest code**, and check the build tools for your system are installed. |
| macOS: missing library errors on start | `brew install glfw openal-soft mpg123` |
| Windows: graphics glitches | Install again with the `gl3` renderer. |
| "No session with that code" | The code is wrong, the host left, or you're on a different relay from the host. |
| "Version mismatch: update reVC" | Everyone needs the same version. Update with the installer and play again. |
| Can't reach the relay | Check it's running, the address and port are right, UDP 7777 is allowed by the firewall, and (for internet play) forwarded on the router or opened in the cloud firewall. Test with `revc-fakeclient --relay <address> --host`. |

Multiplayer is new. If something breaks,
[docs/multiplayer/testing.md](docs/multiplayer/testing.md) lists what to check,
and bug reports are welcome.
