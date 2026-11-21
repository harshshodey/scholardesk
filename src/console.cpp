#include "console.hpp"
#include "literature.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>

namespace scholar {
namespace {
struct Cancel {};
struct EndOfInput {};
std::string fit(std::string text, std::size_t width) {
    if (text.size() > width) text = text.substr(0, width - 2) + "..";
    return text + std::string(width - text.size(), ' ');
}
std::string decimal(double number, int places = 1) {
    std::ostringstream out; out << std::fixed << std::setprecision(places) << number; return out.str();
}
double number(const std::string& value, const std::string& label) {
    std::istringstream in(value); double result;
    if (!(in >> result)) throw std::invalid_argument(label + ": enter a number.");
    in >> std::ws;
    if (!in.eof() || !std::isfinite(result)) throw std::invalid_argument(label + ": enter a finite number.");
    return result;
}
}
Console::Console(Registry& registry, bool color, bool interactive)
    : registry_(registry), color_(color), interactive_(interactive) {}
std::string Console::paint(const std::string& value, const std::string& code) const {
    return color_ ? "\033[" + code + "m" + value + "\033[0m" : value;
}
std::string Console::input(const std::string& prompt) {
    std::cout << "  " << paint(prompt, "38;5;117") << "  " << std::flush;
    std::string value;
    if (!std::getline(std::cin, value)) throw EndOfInput{};
    value = trim(value);
    if (value == "/cancel") throw Cancel{};
    return value;
}
void Console::pause() {
    std::cout << '\n'; input("Press Enter to continue");
}
void Console::header(const std::string& title, const std::string& subtitle) {
    if (interactive_ && color_) std::cout << "\033[2J\033[H";
    std::cout << '\n' << paint("  +------------------------------------------------------------------------------------------+", "38;5;67") << '\n'
              << paint("  |  " + fit("S / D   S C H O L A R D E S K", 71), "1;38;5;117") << paint("CAMPUS CONSOLE    |", "38;5;67") << '\n'
              << paint("  +------------------------------------------------------------------------------------------+", "38;5;67") << "\n\n"
              << "  " << paint(title, "1;97") << '\n'
              << "  " << paint(subtitle, "38;5;245") << "\n\n";
    if (title.find("Delete") == std::string::npos) {
        const std::size_t quote = title == "Overview" ? 0 : title == "Add a student" ? 1 : title == "Update a student" ? 2 :
            title == "Student directory" || title == "Find a student" ? 3 : title == "Academic insights" ? 8 :
            title == "Export directory" ? 6 : title == "The reading shelf" ? 7 : 5;
        std::cout << "  " << paint(scholar::literaryDisplay(quote), "38;5;79") << '\n'
                  << "  " << paint("- " + scholar::literaryCredit(quote), "38;5;245") << "\n\n";
    }
}
void Console::table(std::vector<Student> students) {
    if (students.empty()) { std::cout << "  No students to display. Add a record to get started.\n"; return; }
    std::sort(students.begin(), students.end(), [](const Student& a, const Student& b) {
        return lower(a.name()) < lower(b.name());
    });
    const std::size_t pageSize = 10;
    std::size_t page = 0;
    while (true) {
        std::cout << paint("  " + fit("ID", 13) + fit("STUDENT", 23) + fit("COURSE", 17) + fit("SEM", 5) +
            fit("CGPA", 7) + fit("ATTEND.", 9) + "STANDING", "1;38;5;117") << '\n'
                  << paint("  " + std::string(90, '-'), "38;5;238") << '\n';
        for (std::size_t i = page * pageSize; i < std::min((page + 1) * pageSize, students.size()); ++i) {
            const auto& s = students[i];
            std::cout << "  " << fit(s.id(), 13) << fit(s.name(), 23) << fit(s.course(), 17) << fit(std::to_string(s.semester()), 5)
                      << fit(decimal(s.cgpa(), 2), 7) << fit(decimal(s.attendance()) + "%", 9)
                      << paint(s.standing(), s.standing() == "Needs support" ? "38;5;215" : "38;5;79") << '\n';
        }
        const auto pages = (students.size() + pageSize - 1) / pageSize;
        std::cout << '\n' << "  " << paint(std::to_string(students.size()) + " records  /  Page " + std::to_string(page + 1) + " of " + std::to_string(pages), "38;5;245") << '\n';
        if (pages == 1) break;
        auto command = lower(input("[N] Next  [P] Previous  [Enter] Done >"));
        if (command.empty()) break;
        if (command == "n" && page + 1 < pages) ++page;
        else if (command == "p" && page > 0) --page;
        else std::cout << "  Choose an available page or press Enter.\n";
    }
}
void Console::detail(const Student& s) {
    std::cout << "  " << paint(s.name(), "1;97") << "  " << paint("/ " + s.id(), "38;5;117") << '\n'
              << "  " << std::string(60, '-') << '\n'
              << "  " << fit("Course", 18) << s.course() << '\n'
              << "  " << fit("Semester", 18) << s.semester() << '\n'
              << "  " << fit("Email", 18) << s.email() << '\n'
              << "  " << fit("CGPA", 18) << decimal(s.cgpa(), 2) << " / 10\n"
              << "  " << fit("Attendance", 18) << decimal(s.attendance()) << "%\n"
              << "  " << fit("Academic standing", 18) << paint(s.standing(), s.standing() == "Needs support" ? "38;5;215" : "38;5;79") << '\n';
}
Student Console::form(const Student* original) {
    std::cout << "  " << paint(original ? "Press Enter to keep a field. Type /cancel to discard edits." :
        "Complete all fields. Type /cancel at any prompt to discard this form.", "38;5;245") << "\n\n";
    auto field = [&](const std::string& label, const std::string& current, auto validate) {
        while (true) {
            auto value = input(label + (original ? " [" + current + "]" : "") + " >");
            if (original && value.empty()) return current;
            try { validate(value); return value; }
            catch (const std::exception& e) { std::cout << "  " << paint(e.what(), "38;5;215") << '\n'; }
        }
    };
    auto id = original ? original->id() : field("Student ID", "", [&](const std::string& v) {
        Student(v, "Test", "Test", 1, "a@b.co", 0, 0);
        if (registry_.find(v)) throw std::invalid_argument("That ID already exists. Choose a unique ID.");
    });
    if (original) std::cout << "  " << paint("Student ID: " + id + " (fixed)", "38;5;245") << '\n';
    auto name = field("Full name", original ? original->name() : "", [&](const std::string& v) { Student(id, v, "Test", 1, "a@b.co", 0, 0); });
    auto course = field("Course", original ? original->course() : "", [&](const std::string& v) { Student(id, name, v, 1, "a@b.co", 0, 0); });
    auto semester = field("Semester (1-12)", original ? std::to_string(original->semester()) : "", [&](const std::string& v) {
        double n = number(v, "Semester");
        if (n < 1 || n > 12 || std::floor(n) != n) throw std::invalid_argument("Semester must be a whole number between 1 and 12.");
    });
    auto email = field("Email address", original ? original->email() : "", [&](const std::string& v) { Student(id, name, course, 1, v, 0, 0); });
    bool cgpaChanged = false, attendanceChanged = false;
    auto cgpa = field("CGPA (0-10)", original ? decimal(original->cgpa(), 2) : "", [&](const std::string& v) {
        Student(id, name, course, 1, email, number(v, "CGPA"), 0);
        cgpaChanged = true;
    });
    auto attendance = field("Attendance % (0-100)", original ? decimal(original->attendance()) : "", [&](const std::string& v) {
        Student(id, name, course, 1, email, 0, number(v, "Attendance"));
        attendanceChanged = true;
    });
    return Student(id, name, course, static_cast<int>(number(semester, "Semester")), email,
        original && !cgpaChanged ? original->cgpa() : number(cgpa, "CGPA"),
        original && !attendanceChanged ? original->attendance() : number(attendance, "Attendance"));
}
void Console::roster() {
    header("Student directory", "An alphabetically sorted overview of every student.");
    table(registry_.all()); pause();
}
void Console::add() {
    header("Add a student", "Enter a new student record, then review it before saving.");
    auto student = form();
    std::cout << '\n'; detail(student);
    if (lower(input("Save this student? [y/N] >")) != "y") { std::cout << "  Record discarded.\n"; pause(); return; }
    registry_.add(student);
    std::cout << "\n  " << paint("SUCCESS  Student added and saved to disk.", "1;38;5;79") << '\n'; pause();
}
void Console::search() {
    header("Find a student", "Search across student IDs, names, courses and email addresses.");
    auto query = input("Search >");
    auto results = registry_.search(query);
    std::cout << '\n';
    if (results.size() == 1) detail(results.front());
    else table(results);
    if (results.size() > 1) {
        auto id = input("Enter an ID for full details, or Enter to return >");
        if (!id.empty()) {
            auto found = std::find_if(results.begin(), results.end(), [&](const Student& s) { return lower(s.id()) == lower(id); });
            if (found != results.end()) { std::cout << '\n'; detail(*found); }
            else std::cout << "  That ID is not in these search results.\n";
        }
    }
    pause();
}
void Console::update() {
    header("Update a student", "Review the current record, then change only the fields you need.");
    auto id = input("Student ID >");
    const auto* found = registry_.find(id);
    if (!found) { std::cout << "  Student not found. Use Find a student to locate an ID.\n"; pause(); return; }
    const Student original = *found;
    detail(original); std::cout << '\n';
    auto edited = form(&original);
    std::cout << "\n  " << paint("REVIEW CHANGES", "1;38;5;117") << '\n'; detail(edited);
    if (lower(input("Save changes? [y/N] >")) == "y") {
        registry_.update(id, edited); std::cout << "  " << paint("SUCCESS  Changes saved.", "1;38;5;79") << '\n';
    } else std::cout << "  Changes discarded.\n";
    pause();
}
void Console::remove() {
    header("Delete a student", "Check the details carefully before removing a record.");
    auto id = input("Student ID >");
    const auto* found = registry_.find(id);
    if (!found) { std::cout << "  Student not found.\n"; pause(); return; }
    detail(*found);
    const auto expected = found->id();
    std::cout << "\n  " << paint("This removes the student from the active directory.", "38;5;215") << '\n';
    if (input("Type " + expected + " to confirm, or Enter to cancel >") == expected) {
        registry_.remove(id); std::cout << "  " << paint("SUCCESS  Student deleted. Previous data is in the .bak file.", "1;38;5;79") << '\n';
    } else std::cout << "  Deletion cancelled.\n";
    pause();
}
void Console::insights() {
    header("Academic insights", "A quick view of performance, attendance and course distribution.");
    const auto& students = registry_.all();
    if (students.empty()) { std::cout << "  Add students to see academic insights.\n"; pause(); return; }
    double cgpa = 0, attendance = 0;
    int support = 0, distinction = 0;
    std::map<std::string, int> courses;
    for (const auto& s : students) {
        cgpa += s.cgpa(); attendance += s.attendance(); ++courses[s.course()];
        support += s.standing() == "Needs support"; distinction += s.standing() == "Distinction";
    }
    std::cout << "  " << fit("Average CGPA", 24) << paint(decimal(cgpa / students.size(), 2) + " / 10", "1;38;5;117") << '\n'
              << "  " << fit("Average attendance", 24) << decimal(attendance / students.size()) << "%\n"
              << "  " << fit("Distinction", 24) << distinction << " students\n"
              << "  " << fit("Needs support", 24) << paint(std::to_string(support) + " students", "38;5;215") << "\n\n"
              << "  " << paint("COURSE DISTRIBUTION", "1;38;5;117") << '\n';
    for (const auto& entry : courses) {
        auto blocks = static_cast<std::size_t>(30.0 * entry.second / students.size());
        std::cout << "  " << fit(entry.first, 26) << paint(std::string(blocks, '#'), "38;5;79")
                  << std::string(30 - blocks, ' ') << "  " << entry.second << '\n';
    }
    std::cout << "\n  " << paint("Support: attendance < 75% OR CGPA < 5.0.\n  Distinction: CGPA >= 8.5 AND attendance >= 75%. All others: on track.", "38;5;245") << '\n';
    pause();
}
void Console::exportRecords() {
    header("Export directory", "Create a spreadsheet-friendly CSV copy of your student records.");
    auto value = input("Filename in exports/ [Enter for timestamped file] >");
    if (value.empty()) {
        auto now = std::chrono::system_clock::now().time_since_epoch();
        value = "students-" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(now).count()) + ".csv";
    }
    if (value == "." || value == ".." || value.find_first_of("/\\:<>|?*\"") != std::string::npos)
        throw std::invalid_argument("Use a filename without folders or special path characters.");
    if (std::filesystem::path(value).extension() != ".csv") value += ".csv";
    const auto destination = registry_.path().parent_path().parent_path() / "exports" / value;
    registry_.exportCsv(destination);
    std::cout << "\n  " << paint("SUCCESS  Export created.", "1;38;5;79") << "\n  " << std::filesystem::absolute(destination).string() << '\n'; pause();
}
void Console::readingShelf() {
    header("The reading shelf", "Human-authored passages. Select one to see its context and original source.");
    for (std::size_t i = 0; i < scholar::literaryQuotes().size(); ++i) {
        const auto& q = scholar::literaryQuotes()[i];
        std::cout << "  " << fit(std::to_string(i + 1) + ". " + q.author, 34) << fit(q.text, 53) << '\n';
    }
    std::cout << '\n';
    const auto choice = input("Passage number [1-12], or Enter to return >");
    if (choice.empty()) return;
    const double selected = number(choice, "Passage number");
    if (selected < 1 || selected > 12 || std::floor(selected) != selected) throw std::invalid_argument("Choose a whole number from 1 to 12.");
    const auto index = static_cast<std::size_t>(selected) - 1;
    const auto& q = scholar::literaryQuotes()[index];
    std::cout << '\n' << "  " << paint(scholar::literaryDisplay(index), "1;38;5;79") << '\n'
              << "  " << q.author << " | " << q.work << '\n'
              << "  " << q.locator << "\n\n  " << q.context << "\n\n  Original: " << q.url << '\n';
    pause();
}
void Console::run() {
    while (true) {
        try {
            header("Overview", "Student records and academic progress. Built with C++17.");
            const auto& students = registry_.all();
            double average = 0; int support = 0;
            for (const auto& s : students) { average += s.cgpa(); support += s.standing() == "Needs support"; }
            std::cout << paint("  +--------------------------+--------------------------+------------------------------------+", "38;5;67") << '\n'
                      << "  | " << fit("ENROLLED", 25) << "| " << fit("AVERAGE CGPA", 25) << "| " << fit("NEEDS SUPPORT", 35) << "|\n"
                      << "  | " << paint(fit(std::to_string(students.size()) + " students", 25), "1;38;5;117")
                      << "| " << paint(fit(students.empty() ? "--" : decimal(average / students.size(), 2) + " / 10", 25), "1;38;5;79")
                      << "| " << paint(fit(std::to_string(support) + " students", 35), "1;38;5;215") << "|\n"
                      << paint("  +--------------------------+--------------------------+------------------------------------+", "38;5;67") << "\n\n"
                      << "  " << paint("WORKSPACE", "1;38;5;245") << "\n\n"
                      << "  " << paint("[1]", "1;38;5;117") << " Student directory         " << paint("[2]", "1;38;5;117") << " Add a student\n\n"
                      << "  " << paint("[3]", "1;38;5;117") << " Find a student            " << paint("[4]", "1;38;5;117") << " Update a student\n\n"
                      << "  " << paint("[5]", "1;38;5;117") << " Delete a student          " << paint("[6]", "1;38;5;117") << " Academic insights\n\n"
                      << "  " << paint("[7]", "1;38;5;117") << " Export to CSV             " << paint("[0]", "1;38;5;245") << " Save & exit\n\n"
                      << "  " << paint("[8]", "1;38;5;117") << " Reload saved directory\n\n"
                      << "  " << paint("[9]", "1;38;5;117") << " The reading shelf\n\n"
                      << "  " << paint("AUTO-SAVE ON  /  Changes are saved after each operation.", "38;5;245") << '\n'
                      << "  " << paint("Data: " + registry_.path().string(), "38;5;245") << "\n\n";
            auto choice = input("Choose an option >");
            if (choice == "0") { std::cout << "\n  " << paint("All saved. See you next time.", "1;38;5;79") << "\n\n"; return; }
            if (choice == "1") roster();
            else if (choice == "2") add();
            else if (choice == "3") search();
            else if (choice == "4") update();
            else if (choice == "5") remove();
            else if (choice == "6") insights();
            else if (choice == "7") exportRecords();
            else if (choice == "8") { registry_.reload(); std::cout << "  Directory reloaded from disk.\n"; pause(); }
            else if (choice == "9") readingShelf();
            else { std::cout << "  Choose a number from 0 to 9.\n"; pause(); }
        } catch (const EndOfInput&) { std::cout << "\n  Input closed. All completed changes are saved.\n"; return; }
        catch (const Cancel&) { std::cout << "\n  Action cancelled.\n"; }
        catch (const std::exception& e) {
            std::cout << "\n  " << paint(std::string("Could not complete action: ") + e.what(), "38;5;215") << '\n';
            try { pause(); } catch (const EndOfInput&) { return; } catch (const Cancel&) {}
        }
    }
}
}
