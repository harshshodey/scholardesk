#include "registry.hpp"
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif

namespace scholar {
namespace fs = std::filesystem;
namespace {
constexpr const char* header = "SCHOLARDESK 1";
// A short-lived OS lock serializes cooperating writers. OS handles release on crashes.
class WriteLock {
#ifdef _WIN32
    HANDLE handle_;
#else
    int handle_;
#endif
public:
    explicit WriteLock(fs::path path) {
        path += ".lock";
#ifdef _WIN32
        handle_ = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (handle_ == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot save right now. Another instance may be saving, or the folder is not writable. Try again.");
#else
        handle_ = ::open(path.c_str(), O_CREAT | O_RDWR, 0600);
        if (handle_ < 0) throw std::runtime_error("Cannot open the storage lock. Check folder permissions.");
        if (flock(handle_, LOCK_EX | LOCK_NB) != 0) { ::close(handle_); throw std::runtime_error("Another instance is saving. Try again."); }
#endif
    }
    ~WriteLock() {
#ifdef _WIN32
        CloseHandle(handle_);
#else
        flock(handle_, LOCK_UN); ::close(handle_);
#endif
    }
    WriteLock(const WriteLock&) = delete;
    WriteLock& operator=(const WriteLock&) = delete;
};
std::optional<std::string> readFile(const fs::path& path) {
    if (!fs::exists(path)) return std::nullopt;
    if (!fs::is_regular_file(path)) throw std::runtime_error("The data path must be a regular file, not a folder.");
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Cannot read data file. Check its path and permissions.");
    std::ostringstream bytes; bytes << file.rdbuf();
    if (file.bad()) throw std::runtime_error("Reading the data file failed.");
    return bytes.str();
}
void replaceFile(const fs::path& source, const fs::path& target) {
#ifdef _WIN32
    if (!MoveFileExW(source.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        throw std::runtime_error("Cannot replace data file (Windows error " + std::to_string(GetLastError()) + ").");
#else
    fs::rename(source, target);
#endif
}
std::string csv(std::string value) {
    // Prevent formula evaluation when an exported value is opened in a spreadsheet.
    if (!value.empty() && std::string("=+-@").find(value[0]) != std::string::npos) value.insert(0, "'");
    std::string result = "\"";
    for (char c : value) { if (c == '"') result += '"'; result += c; }
    return result + '"';
}
}
Registry::Registry(fs::path path) : path_(std::move(path)) {
    diskSnapshot_ = readFile(path_);
    if (!diskSnapshot_) return;
    std::istringstream input(*diskSnapshot_);
    std::string line;
    if (!std::getline(input, line) || trim(line) != header)
        throw std::runtime_error("Unrecognized data file. Restore a valid backup before continuing.");
    std::set<std::string> ids;
    std::size_t lineNumber = 1;
    while (std::getline(input, line)) {
        ++lineNumber;
        if (trim(line).empty()) continue;
        try {
            std::istringstream row(line);
            std::string id, name, course, email;
            int semester;
            double cgpa, attendance;
            if (!(row >> std::quoted(id) >> std::quoted(name) >> std::quoted(course) >> semester >>
                std::quoted(email) >> cgpa >> attendance)) throw std::runtime_error("Incomplete record");
            row >> std::ws;
            if (!row.eof()) throw std::runtime_error("Unexpected extra fields");
            Student student(id, name, course, semester, email, cgpa, attendance);
            if (!ids.insert(lower(student.id())).second) throw std::runtime_error("Duplicate student ID");
            students_.push_back(student);
        } catch (const std::exception& e) {
            throw std::runtime_error("Data file line " + std::to_string(lineNumber) + ": " + e.what() +
                                     ". Original file was not changed.");
        }
    }
    if (input.bad()) throw std::runtime_error("Error while reading data file.");
}
void Registry::reload() {
    Registry fresh(path_);
    students_.swap(fresh.students_);
    diskSnapshot_.swap(fresh.diskSnapshot_);
}
const Student* Registry::find(const std::string& id) const {
    const auto key = lower(trim(id));
    auto it = std::find_if(students_.begin(), students_.end(), [&](const Student& s) { return lower(s.id()) == key; });
    return it == students_.end() ? nullptr : &*it;
}
std::vector<Student> Registry::search(const std::string& query) const {
    const auto key = lower(trim(query));
    std::vector<Student> matches;
    for (const auto& s : students_)
        if (lower(s.id() + " " + s.name() + " " + s.course() + " " + s.email()).find(key) != std::string::npos)
            matches.push_back(s);
    return matches;
}
void Registry::commit(std::vector<Student> next) {
    if (!path_.parent_path().empty()) fs::create_directories(path_.parent_path());
    WriteLock lock(path_);
    if (readFile(path_) != diskSnapshot_)
        throw std::runtime_error("Records changed in another window or program. Reload the directory (F5 on desktop, option 8 in console), then retry. Your saved data was not overwritten.");
    auto temporary = path_; temporary += ".tmp";
    bool opened = false;
    try {
        std::ostringstream serialized;
        serialized << header << '\n' << std::setprecision(17);
        for (const auto& s : next)
            serialized << std::quoted(s.id()) << ' ' << std::quoted(s.name()) << ' ' << std::quoted(s.course()) << ' '
                   << s.semester() << ' ' << std::quoted(s.email()) << ' ' << s.cgpa() << ' ' << s.attendance() << '\n';
        std::optional<std::string> nextSnapshot = serialized.str();
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output) throw std::runtime_error("Cannot create temporary data file.");
        opened = true;
        output << *nextSnapshot;
        output.flush();
        if (!output) throw std::runtime_error("Writing failed; existing records were preserved.");
        output.close();
        if (!output) throw std::runtime_error("Closing data file failed.");
        if (fs::exists(path_)) {
            auto backup = path_; backup += ".bak";
            fs::copy_file(path_, backup, fs::copy_options::overwrite_existing);
        }
        replaceFile(temporary, path_);
        students_.swap(next); // Memory changes only after disk succeeds.
        diskSnapshot_.swap(nextSnapshot);
    } catch (...) {
        std::error_code ignored;
        if (opened) fs::remove(temporary, ignored);
        throw;
    }
}
void Registry::add(const Student& student) {
    // Also disallows insertion of the intentionally empty default-constructed object.
    Student validated(student.id(), student.name(), student.course(), student.semester(), student.email(), student.cgpa(), student.attendance());
    if (find(validated.id())) throw std::invalid_argument("This student ID already exists.");
    auto next = students_; next.push_back(validated); commit(std::move(next));
}
void Registry::update(const std::string& id, const Student& student) {
    if (!find(id)) throw std::invalid_argument("Student ID was not found.");
    Student validated(student.id(), student.name(), student.course(), student.semester(), student.email(), student.cgpa(), student.attendance());
    if (lower(trim(id)) != lower(validated.id())) throw std::invalid_argument("Student IDs cannot be changed.");
    auto next = students_;
    for (auto& s : next) if (lower(s.id()) == lower(trim(id))) { s = validated; break; }
    commit(std::move(next));
}
void Registry::remove(const std::string& id) {
    if (!find(id)) throw std::invalid_argument("Student ID was not found.");
    auto next = students_;
    next.erase(std::remove_if(next.begin(), next.end(), [&](const Student& s) { return lower(s.id()) == lower(trim(id)); }), next.end());
    commit(std::move(next));
}
void Registry::exportCsv(const fs::path& destination) const {
    if (!destination.parent_path().empty()) fs::create_directories(destination.parent_path());
    WriteLock lock(destination);
    if (fs::exists(destination)) throw std::runtime_error("Export already exists. Choose a new filename.");
    auto temporary = destination; temporary += ".tmp";
    if (fs::exists(temporary)) throw std::runtime_error("An unfinished export uses this filename. Choose another filename.");
    std::ofstream out(temporary);
    if (!out) throw std::runtime_error("Cannot create export file.");
    try {
    out << "Student ID,Full name,Course,Semester,Email,CGPA,Attendance (%),Standing\n" << std::setprecision(17);
    for (const auto& s : students_)
        out << csv(s.id()) << ',' << csv(s.name()) << ',' << csv(s.course()) << ',' << s.semester() << ','
            << csv(s.email()) << ',' << s.cgpa() << ',' << s.attendance() << ',' << csv(s.standing()) << '\n';
    out.flush();
    if (!out) throw std::runtime_error("Export write failed.");
    out.close();
    if (!out) throw std::runtime_error("Export close failed.");
#ifdef _WIN32
    if (!MoveFileExW(temporary.c_str(), destination.c_str(), MOVEFILE_WRITE_THROUGH))
        throw std::runtime_error("Cannot finish export. An existing file was not overwritten.");
#else
    fs::create_hard_link(temporary, destination); // Atomically fails if another writer created the destination.
    fs::remove(temporary);
#endif
    } catch (...) {
        out.close();
        std::error_code ignored; fs::remove(temporary, ignored);
        throw;
    }
}
}
