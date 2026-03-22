# helldivers2hotkey2 (WIP Conversion)

A lightweight Linux-native hotkey listener and key sequence playback tool, designed to assist with input sequences in Helldivers 2.

At this time it is a work-in-progress conversion from its functional Windows code version. It is currently not fuctional.
See /src/windows/ for the functional windows source version.

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

* g++
* make (optional)

Compile:

```bash
g++ main.cpp -o helldivers2hotkey2
```

---

## ▶️ Run

```bash
./helldivers2hotkey2
```

---

## 🔧 Configuration

Currently loaded via files in current directory.

Example:

1. HotKeys.csv

this handles mapings for keyboard presses to playback sequences.

```csv
// Example mapping
ALT+SHIFT,0,100,Resupply,CTRL_DOWN,DOWN,DOWN,UP,RIGHT,CTRL_UP
```
In this example, pressing the keyboard combination ALT+SHIFT+0, would result in the playback of e follwing key sequence; Hold Down CTRL, DOWN,DOWN,UP,RIGHT, and Relase CTRL, calling in the Resupply stratagem.
The 0,100 was an early attempt at fixing key press timings, which were later implemtned and replaced by 'settings.cfg.

2. settings.cfg

This handles the key press timings and playback speeds in milliseconds delays.

```
modifierHoldMs=150
keyHoldMs=100
postReleaseMs=80
```

---

## Development History

Orginally a Windows 11 Visual Studio c++ console appliction project being converted/rebuilt for native linux use.

It's initial Linux native build is not yet functional. (March 2026)


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
