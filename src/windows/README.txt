HellDivers2HotKey2.cpp

helldivers2hotkey2
A lightweight Windows Console Application hotkey listener and key sequence playback tool, designed to assist with input sequences in Helldivers 2.

✨ What it does
This program listens for a custom hotkey combination presses and plays back a predefined sequence of key presses for stratagems.

Example
Press: ALT + SHIFT + 5

Simulates: UP → DOWN → DOWN → LEFT → LEFT

Useful for quickly executing stratagems input sequences.

🐧 Platform
Built usinfg Visual Studio 2019 on Windows 11 25H2
⚙️ Build
Requirements:

Complie in your Windows compiler of choice.

Turn off Windows hotkey combination for langhuage selection (CTRL+SHIFT) in windows language settings

▶️ Run
cmd: helldivers2hotkey2.exe
🔧 Configuration
Currently loaded via files in current directory.

Example:

HotKeys.csv
this handles mapings for keyboard presses to playback sequences.

// Example mapping
ALT+SHIFT,0,100,Resupply,CTRL_DOWN,DOWN,DOWN,UP,RIGHT,CTRL_UP
In this example, pressing the keyboard combination ALT+SHIFT+0, would result in the playback of e follwing key sequence; Hold Down CTRL, DOWN,DOWN,UP,RIGHT, and Relase CTRL, calling in the Resupply stratagem. The 0,100 was an early attempt at fixing key press timings, which were later implemtned and replaced by 'settings.cfg.

settings.cfg
This handles the key press timings and playback speeds in milliseconds delays.

modifierHoldMs=150
keyHoldMs=100
postReleaseMs=80

Contributing
This verion is considered complete, but ideas and contribution are welcome.

License
MIT License © 2026 davemcree@gmail.com
