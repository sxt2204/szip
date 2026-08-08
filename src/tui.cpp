#include "tui.hpp"
#include <iostream>
#include <vector>
#include <string>

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace sxzip {
namespace tui {

const int KEY_UP = 1000;
const int KEY_DOWN = 1001;
const int KEY_ENTER = 1002;
const int KEY_ESC = 1003;

int get_key() {
#ifdef _WIN32
    int ch = _getch();
    if (ch == 0 || ch == 224) {
        int ex = _getch();
        if (ex == 72) return KEY_UP;
        if (ex == 80) return KEY_DOWN;
    }
    if (ch == 13) return KEY_ENTER;
    if (ch == 27) return KEY_ESC;
    return ch;
#else
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    newt.c_cc[VMIN] = 1;
    newt.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    int ch = getchar();
    if (ch == 27) {
        newt.c_cc[VMIN] = 0;
        newt.c_cc[VTIME] = 1; 
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        int seq1 = getchar();
        int seq2 = getchar();
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        if (seq1 == 91) {
            if (seq2 == 65) return KEY_UP;
            if (seq2 == 66) return KEY_DOWN;
        }
        return KEY_ESC;
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    if (ch == 10) return KEY_ENTER;
    return ch;
#endif
}

void clear_screen() {
    std::cout << "\033[2J\033[1;1H";
}

int run() {
    std::vector<std::string> options = {
        "  Compress a file/directory (-c)",
        "  Decompress an archive (-d)",
        "  View Archive Info (-i)",
        "  Evaluate Optimal Threshold (-Ev)",
        "  Exit"
    };
    int selected = 0;

    while (true) {
        clear_screen();
        std::cout << "\033[1;36m========================================\033[0m\n";
        std::cout << "\033[1;37m        Sxzip Interactive Menu          \033[0m\n";
        std::cout << "\033[1;36m========================================\033[0m\n\n";

        for (size_t i = 0; i < options.size(); ++i) {
            if (i == (size_t)selected) {
                std::cout << "\033[1;32m> " << options[i] << " \033[0m\n";
            } else {
                std::cout << "  " << options[i] << "\n";
            }
        }
        std::cout << "\n\033[90m(Use UP/DOWN arrows to navigate, ENTER to select)\033[0m\n";

        int key = get_key();
        if (key == KEY_UP) {
            selected = (selected - 1 + options.size()) % options.size();
        } else if (key == KEY_DOWN) {
            selected = (selected + 1) % options.size();
        } else if (key == KEY_ESC) {
            clear_screen();
            break;
        } else if (key == KEY_ENTER) {
            clear_screen();
            if (selected == 4) { // Exit
                break;
            }

            std::string input_path;
            std::string output_path;
            std::string password;

            if (selected == 0) {
                std::cout << "\033[1;36m[Compress Mode]\033[0m\n";
                std::cout << "Enter input path: ";
                std::getline(std::cin, input_path);
                if (input_path.empty()) continue;
                std::cout << "Enter output path (default: " << input_path << ".sxz): ";
                std::getline(std::cin, output_path);
                if (output_path.empty()) output_path = input_path + ".sxz";
                std::cout << "Enter password (leave empty for none): ";
                std::getline(std::cin, password);
                
                std::vector<std::string> args = {"sxzip", "-c", input_path, output_path};
                if (!password.empty()) {
                    args.push_back("-P");
                    args.push_back(password);
                }
                execute_cli(args);
            } else if (selected == 1) {
                std::cout << "\033[1;36m[Decompress Mode]\033[0m\n";
                std::cout << "Enter input archive path: ";
                std::getline(std::cin, input_path);
                if (input_path.empty()) continue;
                std::cout << "Enter output path: ";
                std::getline(std::cin, output_path);
                if (output_path.empty()) continue;
                std::cout << "Enter password (if any): ";
                std::getline(std::cin, password);

                std::vector<std::string> args = {"sxzip", "-d", input_path, output_path};
                if (!password.empty()) {
                    args.push_back("-P");
                    args.push_back(password);
                }
                execute_cli(args);
            } else if (selected == 2) {
                std::cout << "\033[1;36m[Info Mode]\033[0m\n";
                std::cout << "Enter input archive path: ";
                std::getline(std::cin, input_path);
                if (input_path.empty()) continue;

                std::vector<std::string> args = {"sxzip", "-i", input_path};
                execute_cli(args);
            } else if (selected == 3) {
                std::cout << "\033[1;36m[Evaluate Mode]\033[0m\n";
                std::cout << "Enter input path: ";
                std::getline(std::cin, input_path);
                if (input_path.empty()) continue;

                std::vector<std::string> args = {"sxzip", "-Ev", input_path};
                execute_cli(args);
            }

            std::cout << "\n\033[1;32mPress any key to return to menu...\033[0m\n";
            get_key();
        }
    }
    return 0;
}

} // namespace tui
} // namespace sxzip
