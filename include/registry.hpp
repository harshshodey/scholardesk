#pragma once
#include "student.hpp"
#include <filesystem>
#include <optional>
#include <vector>

namespace scholar {
// Objects live in a dynamic vector; this class owns all CRUD and file handling.
class Registry {
    std::filesystem::path path_;
    std::vector<Student> students_;
    std::optional<std::string> diskSnapshot_;
    void commit(std::vector<Student> next);
public:
    explicit Registry(std::filesystem::path path);
    void reload();
    const std::vector<Student>& all() const { return students_; }
    const std::filesystem::path& path() const { return path_; }
    const Student* find(const std::string& id) const;
    std::vector<Student> search(const std::string& query) const;
    void add(const Student& student);
    void update(const std::string& id, const Student& student);
    void remove(const std::string& id);
    void exportCsv(const std::filesystem::path& destination) const;
};
}
