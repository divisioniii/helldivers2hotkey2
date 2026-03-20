#include <windows.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <thread>
#include <string>
#include <cctype>
#include <algorithm>
#include <tuple>
#include <iomanip>

// ----------------------------
// Action Model
// ----------------------------

enum class ActionType {
    KeyPress,
    KeyDown,
    KeyUp,
    Delay
};

struct Action {
    ActionType type;
    WORD key = 0;
    int delayMs = 0;
};

// ----------------------------
// Hotkey Struct
// ----------------------------

struct Hotkey {
    bool ctrl = false;
    bool alt = false;
    bool shift = false;
    int key = 0; // Number key 0-9

    bool operator<(const Hotkey& other) const {
        return std::tie(ctrl, alt, shift, key) < std::tie(other.ctrl, other.alt, other.shift, other.key);
    }
};

// ----------------------------
// Hotkey Data Struct
// ----------------------------

struct HotkeyData {
    std::string description;
    std::vector<Action> actions;
};

// ----------------------------
// Timing Settings Struct
// ----------------------------

struct TimingSettings {
    int modifierHoldMs = 80;
    int keyHoldMs = 80;
    int postReleaseMs = 50;
};

// ----------------------------
// Helpers
// ----------------------------

WORD GetVirtualKey(const std::string& keyName) {
    std::string k = keyName;
    std::transform(k.begin(), k.end(), k.begin(), ::toupper);

    if (k == "CTRL") return VK_CONTROL;
    if (k == "SHIFT") return VK_SHIFT;
    if (k == "ALT") return VK_MENU;
    if (k == "UP") return VK_UP;
    if (k == "DOWN") return VK_DOWN;
    if (k == "LEFT") return VK_LEFT;
    if (k == "RIGHT") return VK_RIGHT;
    if (k == "F4") return VK_F4;

    // Single alphanumeric -> use VkKeyScan
    if (k.length() == 1 && isalnum(static_cast<unsigned char>(k[0]))) {
        return VkKeyScanA(static_cast<char>(toupper(k[0]))) & 0xFF;
    }

    return 0;
}

Action ParseTokenToAction(const std::string& token) {
    Action action;

    auto upPos = token.find("_UP");
    auto downPos = token.find("_DOWN");

    if (downPos != std::string::npos) {
        std::string base = token.substr(0, downPos);
        action.type = ActionType::KeyDown;
        action.key = GetVirtualKey(base);
    }
    else if (upPos != std::string::npos) {
        std::string base = token.substr(0, upPos);
        action.type = ActionType::KeyUp;
        action.key = GetVirtualKey(base);
    }
    else if (token == "DELAY") {
        action.type = ActionType::Delay;
        action.delayMs = 100; // token-only default
    }
    else {
        action.type = ActionType::KeyPress;
        action.key = GetVirtualKey(token);
    }

    return action;
}

std::vector<std::string> TokenizeCSVLine(const std::string& line) {
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string token;

    while (std::getline(ss, token, ',')) {
        size_t start = token.find_first_not_of(" \t\r\n");
        size_t end = token.find_last_not_of(" \t\r\n");
        if (start != std::string::npos)
            token = token.substr(start, end - start + 1);
        else
            token.clear();
        tokens.push_back(token);
    }

    return tokens;
}

// ----------------------------
// CSV Parser
// ----------------------------

std::map<Hotkey, HotkeyData> LoadHotkeysFromCSV(const std::string& filename) {
    std::map<Hotkey, HotkeyData> hotkeys;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open config file: " << filename << std::endl;
        return hotkeys;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        // allow comments starting with #
        size_t firstNonWs = line.find_first_not_of(" \t\r\n");
        if (firstNonWs == std::string::npos) continue;
        if (line[firstNonWs] == '#') continue;

        auto tokens = TokenizeCSVLine(line);
        if (tokens.size() < 5) {
            std::cerr << "Invalid line in config (need >=5 fields): " << line << std::endl;
            continue;
        }

        Hotkey hk;
        std::string modStr = tokens[0];
        std::transform(modStr.begin(), modStr.end(), modStr.begin(), ::toupper);

        if (modStr.find("CTRL") != std::string::npos) hk.ctrl = true;
        if (modStr.find("ALT") != std::string::npos) hk.alt = true;
        if (modStr.find("SHIFT") != std::string::npos) hk.shift = true;

        try {
            hk.key = std::stoi(tokens[1]);
        }
        catch (...) {
            std::cerr << "Invalid key number: " << tokens[1] << std::endl;
            continue;
        }

        int delay = 0;
        try {
            delay = std::stoi(tokens[2]);
        }
        catch (...) {
            std::cerr << "Invalid delay: " << tokens[2] << std::endl;
        }

        HotkeyData data;
        data.description = tokens[3];

        if (delay > 0) {
            data.actions.push_back({ ActionType::Delay, 0, delay });
        }

        for (size_t i = 4; i < tokens.size(); ++i) {
            data.actions.push_back(ParseTokenToAction(tokens[i]));
        }

        hotkeys[hk] = data;
    }

    return hotkeys;
}

// ----------------------------
// Settings Loader
// ----------------------------

TimingSettings LoadTimingSettings(const std::string& filename) {
    TimingSettings settings;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "No settings file found (" << filename << "), using defaults.\n";
        return settings;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        size_t firstNonWs = line.find_first_not_of(" \t\r\n");
        if (firstNonWs == std::string::npos) continue;
        if (line[firstNonWs] == '#') continue;

        auto pos = line.find('=');
        if (pos == std::string::npos) continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        // trim
        auto trim = [](std::string& s) {
            size_t a = s.find_first_not_of(" \t\r\n");
            size_t b = s.find_last_not_of(" \t\r\n");
            if (a == std::string::npos) { s.clear(); return; }
            s = s.substr(a, b - a + 1);
        };
        trim(key); trim(value);

        try {
            int val = std::stoi(value);
            if (key == "modifierHoldMs") settings.modifierHoldMs = val;
            else if (key == "keyHoldMs") settings.keyHoldMs = val;
            else if (key == "postReleaseMs") settings.postReleaseMs = val;
        }
        catch (...) {
            std::cerr << "Invalid value in settings for key '" << key << "': " << value << std::endl;
        }
    }
    return settings;
}

// ----------------------------
// Input Execution (Human-like)
// ----------------------------

void ExecuteAction(const Action& action, const TimingSettings& ts) {
    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = action.key;

    switch (action.type) {
    case ActionType::KeyPress:
        if (action.key != 0) {
            SendInput(1, &input, sizeof(INPUT)); // down
            Sleep(ts.keyHoldMs);
            input.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &input, sizeof(INPUT)); // up
            Sleep(ts.postReleaseMs);
        }
        break;

    case ActionType::KeyDown:
        if (action.key != 0) {
            SendInput(1, &input, sizeof(INPUT));
            Sleep(ts.keyHoldMs);
        }
        break;

    case ActionType::KeyUp:
        if (action.key != 0) {
            input.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &input, sizeof(INPUT));
            Sleep(ts.postReleaseMs);
        }
        break;

    case ActionType::Delay:
        Sleep(action.delayMs);
        break;
    }
}

void ExecuteSequence(const std::vector<Action>& sequence, const TimingSettings& ts) {
    std::vector<WORD> modifiersDown;
    std::vector<Action> mainActions;

    // Separate explicit modifier downs from other actions
    for (const auto& action : sequence) {
        if (action.type == ActionType::KeyDown &&
            (action.key == VK_CONTROL || action.key == VK_MENU || action.key == VK_SHIFT)) {
            modifiersDown.push_back(action.key);
        }
        else {
            mainActions.push_back(action);
        }
    }

    // Press modifiers first (and hold)
    for (WORD mod : modifiersDown) {
        INPUT input = {};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = mod;
        SendInput(1, &input, sizeof(INPUT));
        Sleep(ts.modifierHoldMs);
    }

    // Execute remaining actions with human-like timing
    for (const auto& action : mainActions) {
        ExecuteAction(action, ts);
    }
    Sleep(80); // <-- small settle delay after last action

    // Release modifiers last
    for (WORD mod : modifiersDown) {
        INPUT input = {};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = mod;
        input.ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &input, sizeof(INPUT));
        Sleep(ts.postReleaseMs);
    }
    
}

// ----------------------------
// Hotkey Check
// ----------------------------

bool IsHotkeyPressed(const Hotkey& hk) {
    bool ctrlPressed = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    bool shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    bool altPressed = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

    // '0' + hk.key works for 0..9
    bool numPressed = (GetAsyncKeyState('0' + hk.key) & 0x8000) != 0;

    return
        (!hk.ctrl || ctrlPressed) &&
        (!hk.shift || shiftPressed) &&
        (!hk.alt || altPressed) &&
        numPressed;
}

// ----------------------------
// Main
// ----------------------------

int main() {
    std::cout << "Hotkey Macro Console App (CSV + Description + Human Timing + Configurable Delays)\n\n";

    // Load settings
    TimingSettings ts = LoadTimingSettings("settings.cfg");
    std::cout << "Timing Settings: modifierHoldMs=" << ts.modifierHoldMs
        << ", keyHoldMs=" << ts.keyHoldMs
        << ", postReleaseMs=" << ts.postReleaseMs << "\n\n";

    // Load hotkeys
    auto hotkeys = LoadHotkeysFromCSV("hotkeys.csv");
    if (hotkeys.empty()) {
        std::cerr << "No hotkeys loaded. Check hotkeys.csv and its format.\n";
    }

    // Startup table
    std::cout << "Loaded Hotkeys:\n";
    std::cout << std::left << std::setw(20) << "Modifier+Key" << "Description\n";
    std::cout << "---------------------------------------------\n";
    for (const auto& pair : hotkeys) {
        const Hotkey& hk = pair.first;
        const HotkeyData& data = pair.second;

        std::ostringstream combo;
        if (hk.ctrl) combo << "CTRL+";
        if (hk.alt) combo << "ALT+";
        if (hk.shift) combo << "SHIFT+";
        combo << hk.key;

        std::cout << std::left << std::setw(20) << combo.str() << data.description << "\n";
    }
    std::cout << "---------------------------------------------\n\n";

    std::cout << "Listening for hotkeys...\n";

    // Main loop
    while (true) {
        for (const auto& pair : hotkeys) {
            const Hotkey& hk = pair.first;
            const HotkeyData& data = pair.second;

            if (IsHotkeyPressed(hk)) {
                std::cout << "Hotkey ";
                if (hk.ctrl) std::cout << "CTRL+";
                if (hk.alt) std::cout << "ALT+";
                if (hk.shift) std::cout << "SHIFT+";
                std::cout << hk.key << " (" << data.description << ") triggered.                        \r";

                ExecuteSequence(data.actions, ts);
                Sleep(1000); // debounce so it doesn't re-trigger immediately
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    return 0;
}
