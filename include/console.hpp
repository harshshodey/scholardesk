#pragma once
#include "registry.hpp"

namespace scholar {
class Console {
    Registry& registry_;
    bool color_;
    bool interactive_;
    std::string paint(const std::string& value, const std::string& code) const;
    std::string input(const std::string& prompt);
    void pause();
    void header(const std::string& title, const std::string& subtitle);
    void table(std::vector<Student> students);
    void detail(const Student& student);
    void roster();
    void add();
    void search();
    void update();
    void remove();
    void insights();
    void exportRecords();
    void readingShelf();
    Student form(const Student* original = nullptr);
public:
    Console(Registry& registry, bool color, bool interactive);
    void run();
};
}
