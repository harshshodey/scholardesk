#include "registry.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <limits>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace fs = std::filesystem;
using scholar::Registry;
using scholar::Student;
namespace {
int checks = 0;
void check(bool condition, const std::string& message) {
    ++checks;
    if (!condition) throw std::runtime_error("FAIL: " + message);
}
template <typename F> void rejects(F operation, const std::string& message) {
    bool rejected = false;
    try { operation(); } catch (const std::exception&) { rejected = true; }
    check(rejected, message);
}
std::string contents(const fs::path& path) {
    std::ifstream in(path); std::ostringstream out; out << in.rdbuf(); return out.str();
}
Student sample(std::string id = "STU-1", std::string name = "Asha Rao", double cgpa = 8.9, double attendance = 91) {
    return Student(id, name, "Computer Science", 3, "asha@example.com", cgpa, attendance);
}
}
int main() {
    const auto folder = fs::temp_directory_path() / ("scholardesk-tests-" + std::to_string(
        std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    fs::create_directories(folder);
    try {
        const auto path = folder / "data" / "students.sdb";
        Registry registry(path);
        check(registry.all().empty(), "fresh registry is empty");
        registry.add(sample());
        check(registry.all().size() == 1 && fs::exists(path), "add saves to disk");
        check(registry.find(" stu-1 ") != nullptr, "ID lookup is case insensitive and trimmed");
        check(registry.find("missing") == nullptr, "missing lookup");
        rejects([&] { registry.add(sample("stu-1")); }, "duplicate ID rejected");
        rejects([&] { registry.add(Student()); }, "empty default object rejected");
        check(registry.search("ASHA").size() == 1, "case insensitive name search");
        check(registry.search("science").size() == 1, "course search");
        check(registry.search("@example").size() == 1, "email search");
        check(registry.search("STU-").size() == 1, "partial ID search");
        check(registry.search("missing").empty(), "empty search results");
        registry.update("stu-1", sample("STU-1", "Asha \"Ace\" Rao", 9.125, 97.25));
        check(registry.find("STU-1")->name() == "Asha \"Ace\" Rao", "update record");
        check(fs::exists(path.string() + ".bak"), "backup created");
        Registry old(path.string() + ".bak");
        check(old.find("STU-1")->name() == "Asha Rao", "backup holds previous revision");
        Registry reload(path);
        check(reload.find("STU-1")->name() == "Asha \"Ace\" Rao", "quoted strings survive reload");
        check(reload.find("STU-1")->cgpa() == 9.125 && reload.find("STU-1")->attendance() == 97.25, "numeric values survive reload");
        rejects([&] { registry.update("missing", sample()); }, "cannot update unknown ID");
        rejects([&] { registry.update("STU-1", sample("STU-2")); }, "ID is immutable");
        check(sample("A", "Test", 9, 74.9).standing() == "Needs support", "attendance risk takes priority");
        check(sample("A", "Test", 4.9, 90).standing() == "Needs support", "CGPA risk");
        check(sample("A", "Test", 8.5, 75).standing() == "Distinction", "distinction boundary");
        check(sample("A", "Test", 5, 75).standing() == "On track", "on track boundary");
        rejects([] { sample("bad id"); }, "invalid ID");
        rejects([] { sample("A", ""); }, "empty name");
        rejects([] { sample("A", "Name\nInjection"); }, "newline rejected");
        rejects([] { sample("A", "Name", 10.1); }, "invalid CGPA");
        rejects([] { sample("A", "Name", 5, -1); }, "invalid attendance");
        rejects([] { Student("A", "Name", "Course", 0, "a@b.co", 5, 80); }, "invalid semester");
        rejects([] { Student("A", "Name", "Course", 1, "invalid", 5, 80); }, "invalid email");
        registry.add(sample("STU-2", "=SUM(A1)"));
        const auto csvPath = folder / "exports" / "students.csv";
        registry.exportCsv(csvPath);
        auto csv = contents(csvPath);
        check(csv.find("Student ID,Full name") == 0, "CSV header");
        check(csv.find("Asha \"\"Ace\"\" Rao") != std::string::npos, "CSV escapes quotes");
        check(csv.find("'=SUM(A1)") != std::string::npos, "CSV neutralizes formulas");
        rejects([&] { registry.exportCsv(csvPath); }, "existing export protected");
        registry.remove("stu-2");
        check(registry.all().size() == 1, "delete updates vector");
        check(Registry(path).find("STU-2") == nullptr, "delete survives reload");
        rejects([&] { registry.remove("missing"); }, "unknown delete rejected");
        registry.remove("STU-1");
        check(Registry(path).all().empty(), "delete final student persists empty state");

        const auto corrupt = folder / "corrupt.sdb";
        { std::ofstream out(corrupt); out << "SCHOLARDESK 1\nmalformed\n"; }
        const auto before = contents(corrupt);
        rejects([&] { Registry bad(corrupt); }, "malformed record refused");
        check(contents(corrupt) == before, "corrupt file unchanged");
        { std::ofstream out(corrupt); out << "WRONG HEADER\n"; }
        rejects([&] { Registry bad(corrupt); }, "bad header refused");
        { std::ofstream out(corrupt); out << "SCHOLARDESK 1\n\"A\" \"Name\" \"Course\" 1 \"a@b.co\" 5 80\n"
            "\"a\" \"Name\" \"Course\" 1 \"a@b.co\" 5 80\n"; }
        rejects([&] { Registry bad(corrupt); }, "duplicate IDs in file refused");
        { std::ofstream out(corrupt); out << "SCHOLARDESK 1\n\"A\" \"Name\" \"Course\" 1 \"a@b.co\" 5 80 unexpected\n"; }
        rejects([&] { Registry bad(corrupt); }, "extra fields refused");

        const auto blocked = folder / "blocked.sdb";
        Registry blockedRegistry(blocked);
        fs::create_directories(blocked.string() + ".tmp");
        rejects([&] { blockedRegistry.add(sample()); }, "unwritable temporary target rejected");
        check(blockedRegistry.all().empty() && !fs::exists(blocked), "failed save leaves memory and disk unchanged");
        check(fs::is_directory(blocked.string() + ".tmp"), "failed save does not delete a pre-existing temporary directory");

        const auto shared = folder / "shared.sdb";
        Registry first(shared), second(shared);
        first.add(sample("FIRST"));
        const auto firstVersion = contents(shared);
        rejects([&] { second.add(sample("SECOND")); }, "stale empty registry cannot overwrite a newly created file");
        check(second.all().empty() && contents(shared) == firstVersion, "stale-save failure preserves memory and disk");
        second.reload(); second.add(sample("SECOND"));
        check(Registry(shared).all().size() == 2, "reload allows safe retry");
        rejects([&] { first.remove("FIRST"); }, "stale deletion cannot remove another writer's changes");
        first.reload();
        check(first.find("SECOND") != nullptr, "reload refreshes existing objects");
        const auto valid = contents(shared);
        { std::ofstream out(shared); out << "CORRUPTED\n"; }
        rejects([&] { first.reload(); }, "malformed reload rejected");
        check(first.all().size() == 2 && first.find("FIRST"), "failed reload preserves previous in-memory directory");
        { std::ofstream out(shared); out << valid; }
        first.reload();
        fs::remove(shared);
        rejects([&] { first.add(sample("THIRD")); }, "externally deleted data is not silently recreated");
        check(!fs::exists(shared), "stale save preserves external deletion");
        first.reload(); check(first.all().empty(), "explicit reload accepts externally deleted directory");
        first.add(sample("THIRD"));
#ifdef _WIN32
        const auto lockPath = fs::path(shared.string() + ".lock");
        HANDLE lock = CreateFileW(lockPath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        check(lock != INVALID_HANDLE_VALUE, "test acquires real Windows writer lock");
        rejects([&] { first.add(sample("FOURTH")); }, "active writer lock blocks competing save");
        check(first.all().size() == 1, "blocked writer leaves vector unchanged");
        CloseHandle(lock);
#endif
        first.add(sample("FOURTH"));
        check(first.all().size() == 2, "second save succeeds after any writer lock is released");
        const auto backupPath = fs::path(shared.string() + ".bak");
        fs::remove(backupPath); fs::create_directory(backupPath);
        const auto beforeBackupFailure = contents(shared);
        rejects([&] { first.add(sample("FIFTH")); }, "backup failure blocks replacement");
        check(first.all().size() == 2 && contents(shared) == beforeBackupFailure, "backup failure leaves vector and active file intact");
        check(!fs::exists(shared.string() + ".tmp"), "failed backup cleans up its own temporary file");
        fs::remove(backupPath);
        first.add(sample("FIFTH")); check(first.all().size() == 3, "retry after backup failure succeeds");

        for (const std::string email : {"a@b..com", "a@-b.com", "a@b-.com", "a..b@c.com", ".a@b.com", "a.@b.com", "a@b_com.net", "a@b.com@c.com"})
            rejects([&] { Student("A", "Name", "Course", 1, email, 8, 90); }, "malformed email rejected: " + email);
        Student plusEmail("A", "Name", "Course", 1, "first.last+tag@sub.example.com", 8, 90);
        check(plusEmail.email() == "first.last+tag@sub.example.com", "plus-tagged email and subdomain accepted");
        rejects([] { sample("A", "Name", std::numeric_limits<double>::quiet_NaN()); }, "NaN CGPA rejected");
        rejects([] { sample("A", "Name", 8, std::numeric_limits<double>::infinity()); }, "infinite attendance rejected");
        check(sample("A", "Name", 0, 0).cgpa() == 0, "minimum metric values accepted");
        check(sample("A", "Name", 10, 100).attendance() == 100, "maximum metric values accepted");
        check(sample(std::string(16, 'A'), std::string(60, 'N')).name().size() == 60, "maximum field lengths accepted");
        rejects([] { sample(std::string(17, 'A')); }, "ID length overflow rejected");
        rejects([] { sample("A", std::string(61, 'N')); }, "name length overflow rejected");
        const auto unfinished = folder / "exports" / "unfinished.csv";
        { std::ofstream out(unfinished.string() + ".tmp"); out << "preexisting"; }
        rejects([&] { first.exportCsv(unfinished); }, "unfinished export filename is protected");
        check(!fs::exists(unfinished) && contents(unfinished.string() + ".tmp") == "preexisting", "failed export leaves no partial CSV and preserves existing temp");
        rejects([&] { Registry invalid(folder); }, "directory cannot be loaded as a data file");
        const auto beforeExport = contents(csvPath);
        rejects([&] { first.exportCsv(csvPath); }, "repeated CSV export is rejected");
        check(contents(csvPath) == beforeExport, "existing CSV remains byte-for-byte unchanged");
        check(!fs::exists(csvPath.string() + ".tmp"), "successful export has no unfinished temporary file");
        std::cout << "PASS: " << checks << " checks (CRUD, persistence, validation, backups, CSV, failure recovery).\n";
        fs::remove_all(folder);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\nTest files: " << folder << '\n';
        return 1;
    }
}
