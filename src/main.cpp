#include "console.hpp"
#include <cstdlib>
#include <iostream>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

int main(int argc, char* argv[]) {
    bool interactive = false;
#ifdef _WIN32
    interactive = _isatty(_fileno(stdout)) && _isatty(_fileno(stdin));
    DWORD mode = 0;
    auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
    bool color = interactive && GetConsoleMode(handle, &mode) && SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    SetConsoleTitleW(L"ScholarDesk | Student Management System");
#else
    interactive = isatty(STDOUT_FILENO) && isatty(STDIN_FILENO);
    bool color = interactive;
#endif
    if (std::getenv("NO_COLOR")) color = false;
    std::filesystem::path data = "data/students.sdb";
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--no-color") color = false;
        else if (arg == "--data" && i + 1 < argc) data = argv[++i];
        else if (arg == "--help") {
            std::cout << "ScholarDesk - C++17 Student Management System\n"
                         "Usage: ScholarDesk [--data PATH] [--no-color]\n"
                         "Run from the project folder. Default data: data/students.sdb\n"
                         "Type /cancel to leave a form. Input uses printable English text.\n";
            return 0;
        } else { std::cerr << "Unknown or incomplete option: " << arg << "\nUse --help for usage.\n"; return 2; }
    }
    try {
        scholar::Registry registry(data);
        scholar::Console console(registry, color, interactive);
        console.run();
    } catch (const std::exception& e) {
        std::cerr << "\nScholarDesk could not start: " << e.what() << "\n"
                     "Check the data path and permissions. See README.md for backup recovery.\n";
        return 1;
    }
}
