# helldivers2hotkey2 (WIP Conversion)

A lightweight Linux-native hotkey listener and key sequence playback tool, designed to assist with input sequences in Helldivers 2.

At this time it is a work-in-progress conversion from its functional Windows code version. It is currently not release ready.
* See /src/windows/ for the functional windows source version.

## ✨ What it does

This program listens for a custom hotkey combination presses and plays back a predefined sequence of key presses for stratagems.

### Example

Press:
ALT + SHIFT + 5

Simulates:
UP → DOWN → DOWN → LEFT → LEFT

Useful for quickly executing stratagems input sequences.

---

## 🐧 Platform

* Linux (tested on Wayland)
* Initial build on Debian 13.2 Wayland KDE Plasma


---

## ⚙️ Build

Requirements:

* libinput → event stream
* xkbcommon → key translation
* (apt install libinput-dev libudev-dev libxkbcommon-dev)

* g++
* make (optional)

Compile:

```bash
g++ -std=c++17 HellDiver2HotKey2.cpp -o HellDiver2HotKey2 -linput -ludev -lxkbcommon
```

Permissions:

```bash
sudo modprobe uinput
sudo chmod 666 /dev/uinput
```


---

## ▶️ Run

```bash
sudo ./helldivers2hotkey2
```

---

## 🔧 Configuration

Currently loaded via files in current directory.

Example:

1. HotKeys.csv

this handles mapings for keyboard presses to playback sequences.

```csv
Modifier,TriggerKey,Descriptor,MacroSequence
ALT+SHIFT,0,"Resupply",DOWN,DOWN,DOWN,UP,RIGHT
ALT+SHIFT,1,"Orbital Barrage",RIGHT,DOWN,LEFT,UP,UP
ALT+SHIFT,9,"Mortar Sentry",DOWN,UP,RIGHT,RIGHT,DOWN
```
In this example, pressing the keyboard combination ALT+SHIFT+0, would result in the playback of e follwing key sequence; Hold Down CTRL, DOWN,DOWN,UP,RIGHT, and Relase CTRL, calling in the Resupply stratagem.
The 0,100 was an early attempt at fixing key press timings, which were later implemtned and replaced by 'settings.cfg.

2. settings.cfg

This handles the key press timings and playback speeds in milliseconds delays.

```
ModifierToHold=150       # optional additional modifier (like LEFTCTRL)
KeyHoldMs=100            # how long each macro key is held
PostReleaseMs=80         # delay after key release before next key

```

---

## Development History

Orginally a Windows 11 Visual Studio c++ console appliction project being converted/rebuilt for native linux use.

It's initial Linux native build is not yet release reasdy. (March 2026)


---


## 🤝 Contributing

Contributions are welcome.

Ideas:

* X11 Support or not
* Wayland compatibility
* UI (CLI or minimal GUI)
* Better keyboard detection. 
* Currently needs sudo. would prefer not to require that.

---

## 📜 License

MIT License (see LICENSE.txt file)
