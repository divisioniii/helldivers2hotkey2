# helldivers2hotkey2

A lightweight Linux-native hotkey listener and key sequence playback tool, designed to assist with input sequences in Helldivers 2.

## ✨ What it does

This program listens for a custom hotkey and simulates a predefined sequence of key presses for stratagems.

### Example

Press:
ALT + SHIFT + 5

Simulates:
UP → DOWN → DOWN → LEFT → LEFT

Useful for quickly executing stratagems input sequences.

---

## 🐧 Platform

* Linux (tested on X11)
* May not work on Wayland without additional configuration

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

Currently loaded via HotKeys.csv in current directory.

Example:

```csv
// Example mapping
ALT+SHIFT,0,100,Resupply,CTRL_DOWN,DOWN,DOWN,UP,RIGHT,CTRL_UP
```
In this example, pressing the keyboard combination ALT+SHIFT+0, would result in the playback of e follwing key sequence; Hold Down CTRL, DOWN,DOWN,UP,RIGHT, and Relase CTRL, calling in the Resupply stratagem.
The 0,100 was an early attempt at fixing key press timings, which were later implemtned and replaced by 'settings.cfg.

settings.cfg

This handles the key press timings and playback speeds in milliseconds delays.

```
modifierHoldMs=150
keyHoldMs=100
postReleaseMs=80
```

---

## Testing envinment notes

Initial build on Debian 13.2 Wayland KDE Plasma


---


## 🤝 Contributing

Contributions are welcome.

Ideas:

* X11 Support or not
* Wayland compatibility
* UI (CLI or minimal GUI)
* Better keyboard detection. 

---

## 📜 License

MIT License (see LICENSE file)
