# ScholarDesk — practical project guide

## Aim

Design a menu-driven student management system using C++ classes, objects, constructors, a dynamic vector and persistent file handling. Users can add, view, search, update and delete records while the application maintains valid data.

The project includes a native Windows desktop interface for presentation and a portable console interface for studying the core program flow. `src/desktop.cpp` uses the Windows API for windows, controls and drawing; both interfaces call the same `Student` and `Registry` classes. Begin with the console files when learning OOP, then study the desktop file as an interface extension.

## Concepts demonstrated

| C++ concept | Implementation | What to explain in a demonstration |
|---|---|---|
| Class | `Student`, `Registry`, `Console` | Each class has one responsibility: a record, record storage, or the interface |
| Object | `Registry registry(data)` and `Console console(...)` in main | An object is an instance of a class with its own state |
| Default constructor | `Student() = default` | Creates an empty object; the registry refuses to save it until replaced with valid data |
| Parameterized constructor | `Student(id, name, course, semester, email, cgpa, attendance)` | Initializes and validates all record fields |
| Constructor initializer list | `Student` and `Registry` constructors | Initializes private members directly |
| Encapsulation | Private Student members with const getters | Callers cannot bypass validation by modifying fields directly |
| Vector | `std::vector<Student> students_` | Grows dynamically and stores student objects by value |
| Copying objects | Edit form starts from a copy of the original record | Cancelling an edit does not mutate the saved record |
| File handling | `ifstream`, `ofstream`, `quoted`, filesystem operations | Reads records on startup and saves them after mutations |
| Algorithms | `find_if`, `remove_if`, `sort`, `transform` | Searches, deletes, sorts and normalizes record data |
| Exceptions | Validation and I/O errors | Prevents invalid records and reports failed operations |
| Const correctness | `const Student* find(...) const` | Read-only access protects registry-owned objects |

## Program flow

1. Read command-line settings and configure terminal colours.
2. Construct the registry and load the data file into a vector.
3. Display dashboard totals and menu options.
4. Read a choice and route to the appropriate operation.
5. Validate a candidate student and request save confirmation.
6. Copy the vector, apply the mutation, write a temporary data file and back up the old file.
7. Replace the active file and swap in the updated vector.
8. Return to the menu, or exit after option 0.

## Record format

```text
SCHOLARDESK 1
"STU-1001" "Aarav Sharma" "Computer Science" 3 "aarav@example.com" 9.1 96
```

The first line identifies the format and its version. Every following nonempty line represents one student. `std::quoted` preserves spaces, quotation marks and backslashes in strings. Floating-point values are written with enough precision for a round trip. This is a custom text file, not a database or CSV. CSV is a separate export format.

## CRUD algorithms and complexity

For `n` records, lookup and substring search scan the vector in O(n) time. Directory display sorts a copy in O(n log n). Addition uses vector append, but validation and writing the whole file make the complete operation O(n). Updates and deletes also take O(n), because they search/copy the vector and rewrite the data file. Registry memory is O(n); a mutation temporarily holds another O(n) vector. This is appropriate for a classroom project and a modest local directory.

## Five-minute presentation

1. Launch the desktop dashboard and explain the three live summary values. Open **Run Console.cmd** instead if demonstrating the numbered-menu flow below; use the matching desktop controls for a graphical presentation.
2. Show the directory and search `science` to demonstrate a partial course search.
3. Add `STU-1009`, a fictional student with CGPA 8.8 and attendance 93. Show validation by entering an invalid semester first, then correct it.
4. Search the new ID and show all fields.
5. Update attendance to 70. Open academic insights and explain why the record now needs support.
6. Export a CSV and show that a separate file was created.
7. Exit and relaunch to demonstrate persistence.
8. Delete the new record by typing the exact ID. Explain that a backup stores the previous revision.

Use a separate data file with `--data data/presentation.sdb` if you want a completely isolated demonstration.

## Practice exercises

1. **Classes and objects:** construct a Student object and print its getters. Try invalid CGPA values and catch the exception.
2. **Constructors:** compare default construction with parameterized construction; explain why an empty default object cannot be saved.
3. **Vectors:** create a vector of three Student objects; use a range-based loop to calculate average CGPA.
4. **Search:** add an exact course filter while retaining the existing substring search.
5. **File handling:** inspect the quoted storage format, save a record, restart the program and verify it loads.
6. **Extension:** add a phone-number field across the class, form, storage, CSV and tests. Introduce a new file-format version so older files can be handled deliberately.
7. **Testing:** add a boundary test for attendance 75% and CGPA 8.5; explain why these thresholds affect standing.

## Viva questions

**Why use a vector instead of a fixed array?** A vector grows when records are added and exposes standard iterators for algorithms.

**Why separate Console from Registry?** Storage rules can be tested without simulating keyboard input, and the interface can change independently.

**Why copy before saving?** If writing fails, the current in-memory records should still match the existing disk file.

**Why use a constructor initializer list?** It directly initializes each member before the constructor body performs validation.

**Why are IDs case insensitive?** `STU-1` and `stu-1` should not accidentally become separate students. The display preserves the entered spelling.

**Why not edit one line in place?** A replacement record may have a different length. Rewriting a temporary file is simpler and avoids corrupting later records.

**What is the role of the backup?** It preserves one previous committed state for manual recovery. It is overwritten by the next successful change.

**How are conflicting edits handled?** Registry stores a snapshot of the loaded file. A short-lived OS lock serializes cooperating writers. Inside that lock, saving checks whether the current bytes still match the snapshot. A mismatch rejects the save without changing the vector or active file. The user reloads and retries. This prevents accidental lost updates; it does not merge concurrent edits.

**How does Undo delete work?** The desktop stores a copy of the most recently deleted Student object in an optional value. Undo adds that object back through the same validated Registry API, so duplicate checks, conflict detection and file persistence still apply. This session feature is separate from the disk backup.

**Does this use inheritance?** No. The classes have distinct responsibilities and use composition. Inheritance is unnecessary for this scope.

**What are the limitations?** Local storage, printable English text, no authentication, no automatic merging of concurrent edits and no institutional grading integration. A production system would need a database and stronger access control.
