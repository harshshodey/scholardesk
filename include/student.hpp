#pragma once
#include <algorithm>
#include <cctype>
#include <cmath>
#include <stdexcept>
#include <string>

namespace scholar {
inline std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    return first == std::string::npos ? "" : value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
}
inline std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}
inline bool cleanText(const std::string& value) {
    return std::all_of(value.begin(), value.end(), [](unsigned char c) { return c >= 32 && c < 127; });
}

// Encapsulation: only validated constructors can create or change a record.
class Student {
    std::string id_, name_, course_, email_;
    int semester_ = 1;
    double cgpa_ = 0, attendance_ = 0;
public:
    Student() = default; // Demonstrates default construction; validate before adding to a registry.
    Student(std::string id, std::string name, std::string course, int semester,
            std::string email, double cgpa, double attendance)
        : id_(trim(id)), name_(trim(name)), course_(trim(course)), email_(trim(email)),
          semester_(semester), cgpa_(cgpa), attendance_(attendance) {
        if (id_.empty() || id_.size() > 16 || !std::all_of(id_.begin(), id_.end(), [](unsigned char c) {
            return std::isalnum(c) || c == '-' || c == '_';
        })) throw std::invalid_argument("Student ID: use 1-16 letters, digits, hyphens or underscores.");
        if (name_.empty() || name_.size() > 60 || !cleanText(name_))
            throw std::invalid_argument("Name: enter 1-60 printable English characters.");
        if (course_.empty() || course_.size() > 40 || !cleanText(course_))
            throw std::invalid_argument("Course: enter 1-40 printable English characters.");
        const auto at = email_.find('@');
        if (email_.size() > 80 || !cleanText(email_) || email_.find(' ') != std::string::npos || at == 0 ||
            at == std::string::npos || email_.find('@', at + 1) != std::string::npos ||
            email_.find('.', at + 2) == std::string::npos || email_.back() == '.')
            throw std::invalid_argument("Email: enter an address such as name@example.com.");
        // Basic dot-atom and domain-label rules; mailbox existence is not checked.
        const auto local = email_.substr(0, at), domain = email_.substr(at + 1);
        if (local.front() == '.' || local.back() == '.' || local.find("..") != std::string::npos ||
            !std::all_of(local.begin(), local.end(), [](unsigned char c) {
                return std::isalnum(c) || std::string(".!#$%&'*+-/=?^_`{|}~").find(static_cast<char>(c)) != std::string::npos;
            })) throw std::invalid_argument("Email: check the part before @.");
        std::size_t start = 0;
        while (start < domain.size()) {
            const auto end = domain.find('.', start);
            const auto part = domain.substr(start, end == std::string::npos ? end : end - start);
            if (part.empty() || part.size() > 63 || part.front() == '-' || part.back() == '-' ||
                !std::all_of(part.begin(), part.end(), [](unsigned char c) { return std::isalnum(c) || c == '-'; }))
                throw std::invalid_argument("Email: check the domain after @.");
            if (end == std::string::npos) break;
            start = end + 1;
        }
        if (semester_ < 1 || semester_ > 12) throw std::invalid_argument("Semester must be between 1 and 12.");
        if (!std::isfinite(cgpa_) || cgpa_ < 0 || cgpa_ > 10) throw std::invalid_argument("CGPA must be between 0 and 10.");
        if (!std::isfinite(attendance_) || attendance_ < 0 || attendance_ > 100)
            throw std::invalid_argument("Attendance must be between 0 and 100.");
    }
    const std::string& id() const { return id_; }
    const std::string& name() const { return name_; }
    const std::string& course() const { return course_; }
    const std::string& email() const { return email_; }
    int semester() const { return semester_; }
    double cgpa() const { return cgpa_; }
    double attendance() const { return attendance_; }
    std::string standing() const {
        if (attendance_ < 75 || cgpa_ < 5) return "Needs support";
        return cgpa_ >= 8.5 ? "Distinction" : "On track";
    }
};
}
