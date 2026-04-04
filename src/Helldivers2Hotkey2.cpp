#include <iostream>
#include <libinput.h>
#include <libudev.h>
#include <unistd.h>
#include <fcntl.h>
#include <xkbcommon/xkbcommon.h>
#include <linux/uinput.h>
#include <cstring>

#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>

// -----------------------------
// libinput callbacks
// -----------------------------
static int open_restricted(const char *path, int flags, void *user_data) {
    int fd = open(path, flags);
    if (fd < 0) perror("open");
    return fd;
}

static void close_restricted(int fd, void *user_data) { close(fd); }

const struct libinput_interface interface = {
    .open_restricted = open_restricted,
    .close_restricted = close_restricted
};

// -----------------------------
// Key mapping
// -----------------------------
std::unordered_map<std::string, int> key_map = {
    {"ALT", KEY_RIGHTALT},
    {"SHIFT", KEY_RIGHTSHIFT},
    {"LEFTCTRL", KEY_LEFTCTRL},

    {"0", KEY_0}, {"1", KEY_1}, {"2", KEY_2}, {"3", KEY_3},
    {"4", KEY_4}, {"5", KEY_5}, {"6", KEY_6}, {"7", KEY_7},
    {"8", KEY_8}, {"9", KEY_9},

    {"UP", KEY_UP}, {"DOWN", KEY_DOWN}, {"LEFT", KEY_LEFT}, {"RIGHT", KEY_RIGHT}
};

// -----------------------------
// Macro settings
// -----------------------------
struct MacroSettings {
    int modifier_hold_ms = 150;
    int key_hold_ms = 100;
    int post_release_ms = 80;
};

MacroSettings load_settings(const std::string& file) {
    MacroSettings s;
    std::ifstream f(file);
    std::string line;
    while (std::getline(f, line)) {
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        std::string key = line.substr(0, pos);
        int value = std::stoi(line.substr(pos + 1));

        if (key == "ModifierToHold") s.modifier_hold_ms = value;
        else if (key == "KeyHoldMs") s.key_hold_ms = value;
        else if (key == "PostRelease") s.post_release_ms = value;
    }
    return s;
}

// -----------------------------
// Hotkey structures
// -----------------------------
struct Hotkey {
    std::vector<int> modifiers;
    int trigger_key;
    std::string descriptor;
    std::vector<int> macro_sequence;
    bool active = false;
};

// -----------------------------
// CSV loader
// -----------------------------
std::vector<Hotkey> load_hotkeys(const std::string& filename) {
    std::vector<Hotkey> hotkeys;
    std::ifstream f(filename);
    std::string line;

    std::getline(f, line); // skip header

    while (std::getline(f, line)) {
        std::stringstream ss(line);
        std::string mod_str, trig_str, desc_str, macro_rest;

        std::getline(ss, mod_str, ',');
        std::getline(ss, trig_str, ',');
        std::getline(ss, desc_str, ',');
        std::getline(ss, macro_rest);

        Hotkey hk;

        // parse modifiers
        std::stringstream ms(mod_str);
        std::string k;
        while (std::getline(ms, k, '+')) hk.modifiers.push_back(key_map[k]);

        // trigger key
        hk.trigger_key = key_map[trig_str];

        // descriptor
        hk.descriptor = desc_str;

        // macro sequence
        std::stringstream seq_ss(macro_rest);
        while (std::getline(seq_ss, k, ',')) hk.macro_sequence.push_back(key_map[k]);

        hotkeys.push_back(hk);
    }

    return hotkeys;
}

// -----------------------------
// uinput setup
// -----------------------------
int setup_uinput() {
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    for (int i = 0; i < 256; i++) ioctl(fd, UI_SET_KEYBIT, i);

    struct uinput_setup us{};
    us.id.bustype = BUS_USB;
    us.id.vendor = 0x1234;
    us.id.product = 0x5678;
    strcpy(us.name, "virtual-keyboard");

    ioctl(fd, UI_DEV_SETUP, &us);
    ioctl(fd, UI_DEV_CREATE);
    sleep(1);
    return fd;
}

// -----------------------------
// Send key event
// -----------------------------
void send_key(int fd, int key, int val) {
    struct input_event ev{};
    ev.type = EV_KEY;
    ev.code = key;
    ev.value = val;
    write(fd, &ev, sizeof(ev));

    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    ev.value = 0;
    write(fd, &ev, sizeof(ev));
}

// -----------------------------
// Play macro with optional modifier hold
// -----------------------------
void play_macro(int fd, const std::vector<int>& macro, const MacroSettings& s) {
    if (s.modifier_hold_ms > 0) send_key(fd, KEY_LEFTCTRL, 1);

    for (auto key : macro) {
        send_key(fd, key, 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(s.key_hold_ms));
        send_key(fd, key, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(s.post_release_ms));
    }

    if (s.modifier_hold_ms > 0) send_key(fd, KEY_LEFTCTRL, 0);
}

// -----------------------------
int main() {
    auto hotkeys = load_hotkeys("hotkeys.csv");
    auto settings = load_settings("settings.cfg");
    int uinput_fd = setup_uinput();

    struct udev *udev = udev_new();
    struct libinput *li = libinput_udev_create_context(&interface, nullptr, udev);
    libinput_udev_assign_seat(li, "seat0");

    // xkb setup (optional, for key names/printing)
    struct xkb_context *ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    struct xkb_rule_names names{};
    names.rules  = getenv("XKB_DEFAULT_RULES");
    names.model  = getenv("XKB_DEFAULT_MODEL");
    names.layout = getenv("XKB_DEFAULT_LAYOUT");

    if (!names.rules)  names.rules  = "evdev";
    if (!names.model)  names.model  = "pc105";
    if (!names.layout) names.layout = "us";

    struct xkb_keymap *keymap = xkb_keymap_new_from_names(ctx, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
    struct xkb_state *state = xkb_state_new(keymap);

    std::unordered_set<uint32_t> pressed_keys;

    std::cout << "Hotkey listener running...\n";

    while (true) {
        libinput_dispatch(li);

        struct libinput_event *event;
        while ((event = libinput_get_event(li)) != nullptr) {
            if (libinput_event_get_type(event) == LIBINPUT_EVENT_KEYBOARD_KEY) {
                auto *kb = libinput_event_get_keyboard_event(event);
                uint32_t code = libinput_event_keyboard_get_key(kb);
                uint32_t keycode = code + 8;
                auto state_key = libinput_event_keyboard_get_key_state(kb);

                if (state_key == LIBINPUT_KEY_STATE_PRESSED) {
                    pressed_keys.insert(code);
                    xkb_state_update_key(state, keycode, XKB_KEY_DOWN);
                } else {
                    pressed_keys.erase(code);
                    xkb_state_update_key(state, keycode, XKB_KEY_UP);
                }

                // Hotkey detection
                for (auto& hk : hotkeys) {
                    bool match = true;
                    for (int m : hk.modifiers) if (!pressed_keys.count(m)) { match = false; break; }
                    if (!pressed_keys.count(hk.trigger_key)) match = false;

                    if (match && !hk.active) {
                        hk.active = true;
                        std::cout << "🔥 HOTKEY DETECTED: " << hk.descriptor << "\n";
                        std::thread(play_macro, uinput_fd, hk.macro_sequence, settings).detach();
                    }
                    if (!match) hk.active = false;
                }
            }

            libinput_event_destroy(event);
        }

        usleep(1000);
    }

    return 0;
}

