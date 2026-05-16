#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <thread>

// ---------- CONFIG ----------
std::string inputDevice;

// ---------- MACRO STRUCT ----------
struct Macro {
    std::vector<int> sequence;
    std::string name;
};

std::map<int, Macro> macros;

// ---------- KEY MAP ----------
std::map<std::string, int> keyMap = {
    {"UP", KEY_UP},
    {"DOWN", KEY_DOWN},
    {"LEFT", KEY_LEFT},
    {"RIGHT", KEY_RIGHT},

    {"F1", KEY_F1}, {"F2", KEY_F2}, {"F3", KEY_F3},
    {"F4", KEY_F4}, {"F5", KEY_F5}, {"F6", KEY_F6},
    {"F7", KEY_F7}, {"F8", KEY_F8}, {"F9", KEY_F9},
    {"F10", KEY_F10}, {"F11", KEY_F11}, {"F12", KEY_F12}
};

// ---------- EMIT ----------
void emit(int fd, int type, int code, int val) {
    input_event ie{};
    ie.type = type;
    ie.code = code;
    ie.value = val;
    write(fd, &ie, sizeof(ie));
}

// ---------- PLAYBACK ----------
void playMacro(int uinput_fd, const std::vector<int>& sequence) {
    const int CTRL_PRE_DELAY_US  = 40000;
    const int KEY_HOLD_US        = 50000;
    const int KEY_GAP_US         = 40000;
    const int CTRL_POST_DELAY_US = 40000;

    // CTRL down
    emit(uinput_fd, EV_KEY, KEY_LEFTCTRL, 1);
    emit(uinput_fd, EV_SYN, SYN_REPORT, 0);
    usleep(CTRL_PRE_DELAY_US);

    for (int key : sequence) {
        emit(uinput_fd, EV_KEY, key, 1);
        emit(uinput_fd, EV_SYN, SYN_REPORT, 0);

        usleep(KEY_HOLD_US);

        emit(uinput_fd, EV_KEY, key, 0);
        emit(uinput_fd, EV_SYN, SYN_REPORT, 0);

        usleep(KEY_GAP_US);
    }

    usleep(CTRL_POST_DELAY_US);

    // CTRL up
    emit(uinput_fd, EV_KEY, KEY_LEFTCTRL, 0);
    emit(uinput_fd, EV_SYN, SYN_REPORT, 0);
}

// ---------- CONFIG LOADER ----------
void loadConfig(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Failed to open config file\n";
        exit(1);
    }

    std::string line;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        // device=
        if (line.rfind("device=", 0) == 0) {
            inputDevice = line.substr(7);
            continue;
        }

        // macro F6: UP UP DOWN DOWN, "Resupply"
        if (line.rfind("macro ", 0) == 0) {
            std::istringstream iss(line);

            std::string tmp, triggerStr;
            iss >> tmp;        // macro
            iss >> triggerStr; // F6:

            if (triggerStr.back() == ':')
                triggerStr.pop_back();

            if (!keyMap.count(triggerStr)) {
                std::cerr << "Unknown trigger key: " << triggerStr << "             \r";
                continue;
            }

            int triggerKey = keyMap[triggerStr];

            std::string rest;
            std::getline(iss, rest);

            size_t commaPos = rest.find(',');

            std::string seqPart = rest;
            std::string name = "";

            if (commaPos != std::string::npos) {
                seqPart = rest.substr(0, commaPos);
                name = rest.substr(commaPos + 1);

                // trim spaces + quotes
                size_t firstQuote = name.find('"');
                size_t lastQuote = name.rfind('"');

                if (firstQuote != std::string::npos &&
                    lastQuote != std::string::npos &&
                    lastQuote > firstQuote) {

                    name = name.substr(firstQuote + 1,
                                       lastQuote - firstQuote - 1);
                }
            }

            std::istringstream seqStream(seqPart);
            std::vector<int> sequence;
            std::string key;

            while (seqStream >> key) {
                if (keyMap.count(key)) {
                    sequence.push_back(keyMap[key]);
                } else {
                    std::cerr << "Unknown key: " << key << "              \r";
                }
            }

            macros[triggerKey] = Macro{sequence, name};
        }
    }

    if (inputDevice.empty()) {
        std::cerr << "No input device specified\n";
        exit(1);
    }

    std::cout << "Loaded device: " << inputDevice << "\n";

    for (auto& [k, m] : macros) {
        std::cout << "Macro loaded: "
                  << (m.name.empty() ? std::to_string(k) : m.name)
                  << " (" << m.sequence.size() << " steps)             \n";
    }
}

// ---------- UINPUT ----------
int setupUinput() {
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("uinput open");
        exit(1);
    }

    ioctl(fd, UI_SET_EVBIT, EV_KEY);

    ioctl(fd, UI_SET_KEYBIT, KEY_LEFTCTRL);
    ioctl(fd, UI_SET_KEYBIT, KEY_UP);
    ioctl(fd, UI_SET_KEYBIT, KEY_DOWN);
    ioctl(fd, UI_SET_KEYBIT, KEY_LEFT);
    ioctl(fd, UI_SET_KEYBIT, KEY_RIGHT);

    uinput_setup usetup{};
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor  = 0x1234;
    usetup.id.product = 0x5678;
    strcpy(usetup.name, "Macro Device");

    ioctl(fd, UI_DEV_SETUP, &usetup);
    ioctl(fd, UI_DEV_CREATE);

    sleep(1);
    return fd;
}

// ---------- MAIN ----------
int main() {
    loadConfig("config.txt");

    int input_fd = open(inputDevice.c_str(), O_RDONLY);
    if (input_fd < 0) {
        perror("input device");
        return 1;
    }

    int uinput_fd = setupUinput();

    std::cout << "Listening...\n";

    input_event ev;

    while (true) {
        ssize_t n = read(input_fd, &ev, sizeof(ev));
        if (n != sizeof(ev)) continue;

        if (ev.type == EV_KEY && ev.value == 1) {

            auto it = macros.find(ev.code);

            if (it != macros.end()) {
                std::string name =
                    it->second.name.empty()
                        ? std::to_string(ev.code)
                        : it->second.name;

                std::cout << "\rTrigger: " << name << "                    " << std::flush;

                std::thread([=]() {
                    playMacro(uinput_fd, it->second.sequence);
                }).detach();
            }
        }
    }

    close(input_fd);
    close(uinput_fd);
    return 0;
}
