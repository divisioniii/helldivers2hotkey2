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
#include <chrono>

// Linux input
#include <linux/uinput.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

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
    uint16_t key = 0;
    int delayMs = 0;
};

// ----------------------------

struct Hotkey {
    bool ctrl = false;
    bool alt = false;
    bool shift = false;
    int key = 0;

    bool operator<(const Hotkey& other) const {
        return std::tie(ctrl, alt, shift, key) <
               std::tie(other.ctrl, other.alt, other.shift, other.key);
    }
};

struct HotkeyData {
    std::string description;
    std::vector<Action> actions;
};

struct TimingSettings {
    int modifierHoldMs = 80;
    int keyHoldMs = 80;
    int postReleaseMs = 50;
};

// ----------------------------
// Key Mapping (Linux)
// ----------------------------

uint16_t GetVirtualKey(const std::string& keyName) {
    std::string k = keyName;
    std::transform(k.begin(), k.end(), k.begin(), ::toupper);

    if (k == "CTRL") return KEY_LEFTCTRL;
    if (k == "SHIFT") return KEY_LEFTSHIFT;
    if (k == "ALT") return KEY_LEFTALT;

    if (k == "UP") return KEY_UP;
    if (k == "DOWN") return KEY_DOWN;
    if (k == "LEFT") return KEY_LEFT;
    if (k == "RIGHT") return KEY_RIGHT;

    if (k == "F4") return KEY_F4;

    if (k.length() == 1 && isdigit(k[0]))
        return KEY_0 + (k[0] - '0');

    if (k.length() == 1 && isalpha(k[0]))
        return KEY_A + (k[0] - 'A');

    return 0;
}

// ----------------------------

Action ParseTokenToAction(const std::string& token) {
    Action action;

    if (token.find("_DOWN") != std::string::npos) {
        action.type = ActionType::KeyDown;
        action.key = GetVirtualKey(token.substr(0, token.find("_DOWN")));
    }
    else if (token.find("_UP") != std::string::npos) {
        action.type = ActionType::KeyUp;
        action.key = GetVirtualKey(token.substr(0, token.find("_UP")));
    }
    else if (token == "DELAY") {
        action.type = ActionType::Delay;
        action.delayMs = 100;
    }
    else {
        action.type = ActionType::KeyPress;
        action.key = GetVirtualKey(token);
    }

    return action;
}

// ----------------------------
// CSV Parsing
// ----------------------------

std::vector<std::string> TokenizeCSVLine(const std::string& line) {
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string token;

    while (std::getline(ss, token, ',')) {
        size_t a = token.find_first_not_of(" \t\r\n");
        size_t b = token.find_last_not_of(" \t\r\n");
        if (a != std::string::npos && b != std::string::npos && b >= a)
            token = token.substr(a, b - a + 1);
        else
            token.clear();
        tokens.push_back(token);
    }
    return tokens;
}

// ----------------------------

std::map<Hotkey, HotkeyData> LoadHotkeysFromCSV(const std::string& filename) {
    std::map<Hotkey, HotkeyData> hotkeys;
    std::ifstream file(filename);

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        auto tokens = TokenizeCSVLine(line);
        if (tokens.size() < 5) continue;

        Hotkey hk;
        std::string mod = tokens[0];
        std::transform(mod.begin(), mod.end(), mod.begin(), ::toupper);

        hk.ctrl  = mod.find("CTRL")  != std::string::npos;
        hk.alt   = mod.find("ALT")   != std::string::npos;
        hk.shift = mod.find("SHIFT") != std::string::npos;

        hk.key = std::stoi(tokens[1]);

        HotkeyData data;
        data.description = tokens[3];

        for (size_t i = 4; i < tokens.size(); ++i)
            data.actions.push_back(ParseTokenToAction(tokens[i]));

        hotkeys.emplace(hk, data);
    }
    return hotkeys;
}

// ----------------------------
// uinput helpers
// ----------------------------

void emit(int fd, int type, int code, int val) {
    input_event ev{};
    ev.type = type;
    ev.code = code;
    ev.value = val;
    write(fd, &ev, sizeof(ev));
}

// ----------------------------

int SetupUInput() {
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

    ioctl(fd, UI_SET_EVBIT, EV_KEY);

    for (int i = 0; i < 256; ++i)
        ioctl(fd, UI_SET_KEYBIT, i);

    uinput_setup us{};
    us.id.bustype = BUS_USB;
    strcpy(us.name, "Macro Keyboard");

    ioctl(fd, UI_DEV_SETUP, &us);
    ioctl(fd, UI_DEV_CREATE);

    sleep(1);
    return fd;
}

// ----------------------------
// Key Listener
// ----------------------------

bool keyStates[256] = {false};

void ListenKeyboard(const char* device) {
    int fd = open(device, O_RDONLY);
    input_event ev;

    while (read(fd, &ev, sizeof(ev)) > 0) {
        if (ev.type == EV_KEY)
            keyStates[ev.code] = (ev.value != 0);
    }
}

// ----------------------------

bool IsHotkeyPressed(const Hotkey& hk) {
    bool ctrl  = keyStates[KEY_LEFTCTRL] || keyStates[KEY_RIGHTCTRL];
    bool shift = keyStates[KEY_LEFTSHIFT] || keyStates[KEY_RIGHTSHIFT];
    bool alt   = keyStates[KEY_LEFTALT] || keyStates[KEY_RIGHTALT];

    bool num = keyStates[KEY_0 + hk.key];

    return (!hk.ctrl || ctrl) &&
           (!hk.shift || shift) &&
           (!hk.alt || alt) &&
           num;
}

// ----------------------------
// Execution
// ----------------------------

void ExecuteAction(int fd, const Action& action, const TimingSettings& ts) {
    switch (action.type) {
    case ActionType::KeyPress:
        emit(fd, EV_KEY, action.key, 1);
        emit(fd, EV_SYN, SYN_REPORT, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(ts.keyHoldMs));

        emit(fd, EV_KEY, action.key, 0);
        emit(fd, EV_SYN, SYN_REPORT, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(ts.postReleaseMs));
        break;

    case ActionType::Delay:
        std::this_thread::sleep_for(std::chrono::milliseconds(action.delayMs));
        break;

    default: break;
    }
}

// ----------------------------

void ExecuteSequence(int fd, const std::vector<Action>& seq, const TimingSettings& ts) {
    for (const auto& a : seq)
        ExecuteAction(fd, a, ts);
}

// ----------------------------
// Main
// ----------------------------

int main() {
    std::cout << "Linux Hotkey Macro App\n";

    int fd = SetupUInput();

    std::thread(ListenKeyboard, "/dev/input/event2").detach();

    auto hotkeys = LoadHotkeysFromCSV("hotkeys.csv");
    TimingSettings ts;

    while (true) {
        for (auto& [hk, data] : hotkeys) {
            if (IsHotkeyPressed(hk)) {
                std::cout << "Triggered: " << data.description << "\r";
                ExecuteSequence(fd, data.actions, ts);
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}
