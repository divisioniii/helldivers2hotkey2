# helldivers2hotkey2 (WIP Conversion)

A lightweight Linux-native hotkey listener and key sequence playback tool, designed to assist with input sequences in Helldivers 2.

At this time it is a work-in-progress conversion from its functional Windows code version. 
It is currently functional and in testing and tuning phase as at 16 April 2026.
* See /src/windows/ for the functional windows source version.

## ✨ What it does

This program listens for a custom hotkey combination presses and plays back a predefined sequence of key presses for stratagems.



A lightweight native Linux C++ macro engine that captures keyboard input from `/dev/input` and plays back configurable macros using `/dev/uinput`.

Designed to work on **Wayland and X11** without relying on desktop environment APIs.

---

## ✨ Features

- 🎮 Global keyboard trigger support (Linux input subsystem)
- ⚙️ Configurable macros via simple text file
- ⌨️ Supports arrow-key sequences + modifiers (CTRL wrapper)
- 🏷️ Human-readable macro names
- ⚡ Low-latency input injection using `/dev/uinput`
- 🧵 Non-blocking playback (threaded execution)
- 🪶 No external dependencies

---

## 🧠 How it works

The program:

1. Reads raw keyboard input from `/dev/input/eventX`
2. Matches configured trigger keys (e.g. F6, F12)
3. Injects synthetic input events via `/dev/uinput`
4. Wraps all macro sequences in:

CTRL down → sequence → CTRL up


---


🐧 Platform
Linux (tested on Wayland)
Initial build on Debian 13.2 Wayland KDE Plasma

---

## 📦 Requirements

### Build dependencies
- `g++` (C++17 or newer)
- Linux build tools

Install on Debian 13:

```bash
sudo apt install build-essential
Runtime requirements
Linux kernel with input subsystem enabled
Access to:
/dev/input/event*
/dev/uinput
```

🔐 Permissions setup

Option 1 (quick test)
```bash
sudo ./Helldivers2Hotkey2
```

Option 2 (recommended)

Permissions:

```bash
sudo modprobe uinput
sudo chmod 666 /dev/uinput
```

or

Create udev rules:
```bash
sudo nano /etc/udev/rules.d/99-uinput.rules
```
Add:

```bash
KERNEL=="uinput", MODE="0660", GROUP="input"
KERNEL=="event*", MODE="0660", GROUP="input"
```

Add user to input group:

```bash
sudo usermod -aG input $USER
```

Then reboot or re-login.


⌨️ Finding your keyboard device

List devices:

```bash
ls -l /dev/input/by-id/
```

Example output:

```bash
usb-413c_Dell_KB216_Wired_Keyboard-event-kbd -> ../event3
```

Use this in config:

```bash
device=/dev/input/event3
```


⚙️ Configuration

Create a file named config.txt:

```config.txt
# keyboard device
device=/dev/input/event3

# macros (Avoid game related F key presets)
macro F6: DOWN UP LEFT UP RIGHT DOWN, "F6 - AX/AR-23 Guard Dog"
macro F7: DOWN DOWN LEFT UP RIGHT, "F7 - EAST-17 Expendable Anti-Tank"
macro F8: DOWN UP RIGHT RIGHT UP, "F8 - SA/MG-43 Machine Sentry"
macro F9: DOWN UP RIGHT RIGHT DOWN, "F9 - A/M-12 Mortar Sentry"
macro F10: DOWN DOWN UP RIGHT, "F10 - Resupply"
macro F11: RIGHT DOWN LEFT UP UP, "F11 - Orbital Gatling Barrage"
```


Format Example

* macro <TRIGGER_KEY>: <SEQUENCE>, "<NAME>"
* TRIGGER_KEY → F1–F12 supported
* SEQUENCE → UP, DOWN, LEFT, RIGHT
* NAME → displayed in terminal logs


⚙️ Game configuration settings
* This tool with assume you've got the stratgem call in keybinds in the game set to CTRL HOLD.
eg, to manually call a stratagem, you would hold down CTRL while pressing your desired arrow key combo.
This tool wil handle the CTRL HOLD while performing the arrow combination hotkeys as defined above.



🚀 Build
```bash
g++ -O2 -std=c++17 Helldivers2Hotkey2.cpp -o Helldivers2Hotkey2
```


▶️ Run
```bash
./Helldivers2Hotkey2
```

🖥️ Example output
```bash
Loaded device: /dev/input/event3
Macro loaded: F6 - AX/AR-23 Guard Dog (6 steps)
Macro loaded: F7 - EAST-17 Expendable Anti-Tank (5 steps)
Macro loaded: F8 - SA/MG-43 Machine Sentry (5 steps)
Macro loaded: F9 - A/M-12 Mortar Sentry (5 steps)
Macro loaded: F10 - Resupply (4 steps)
Macro loaded: F11 - Orbital Gatling Barrage (5 steps)

Listening...
Trigger: F10 - Resupply
Trigger: F8 - SA/MG-43 Machine Sentry
```
It is advised to avoid using F1-F5, F12 as these are in use by the game.
Set your own stratagem maco choices. see; 
* https://www.corsair.com/us/en/explorer/gamer/keyboards/helldivers-2-stratagem-codes-complete-list/

⚠️ Notes
* Works on Wayland via kernel-level input injection (uinput)
* Some sandboxed applications (Flatpak/Snap) may or may not ignore synthetic input (untested)
* Anti-cheat systems in games may block or flag injected input. Helldivers1 is ok with it (Tested ok)
* Requires access to raw input devices (/dev/input/event*) as noted above

🧩 Limitations / Design Choices
* No GUI (CLI only)
* No hot-reload (restart required for config changes)
* No global hotkey interception via Wayland (by design)
* Keyboard Device must be manually selected (For now)


🔮 Future ideas
* Pending Auto detection for main/default keyboard


🤝 Contributing
* Contributions are welcome.


📜 License

MIT License  (see LICENSE.txt file)

