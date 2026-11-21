// Included only by the dedicated SCHOLAR_DESKTOP_TESTS build, never by the shipped app.
// Drives the real HWND controls, WM_COMMAND handlers, modal loop and Registry together.
int desktopChecks = 0;
void desktopCheck(bool condition, const char* message) {
    ++desktopChecks;
    if (!condition) throw std::runtime_error(std::string("FAIL: ") + message);
}
bool shown(HWND window) { return (GetWindowLongPtrW(window, GWL_STYLE) & WS_VISIBLE) != 0; }
std::wstring emptyMarkup() {
    NMLVEMPTYMARKUP notification{}; notification.hdr.hwndFrom = list; notification.hdr.code = LVN_GETEMPTYMARKUP;
    SendMessageW(mainWindow, WM_NOTIFY, 30, reinterpret_cast<LPARAM>(&notification));
    return notification.szMarkup;
}
void command(int id) { SendMessageW(mainWindow, WM_COMMAND, id, 0); }
void filter(int index) {
    SendMessageW(filterBox, CB_SETCURSEL, index, 0);
    SendMessageW(mainWindow, WM_COMMAND, MAKEWPARAM(12, CBN_SELCHANGE), reinterpret_cast<LPARAM>(filterBox));
}
void selectRow(int index) { ListView_SetItemState(list, index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED); }
void sortBy(int column) {
    NMLISTVIEW notification{}; notification.hdr.hwndFrom = list; notification.hdr.code = LVN_COLUMNCLICK; notification.iSubItem = column;
    SendMessageW(mainWindow, WM_NOTIFY, 30, reinterpret_cast<LPARAM>(&notification));
}
void driveForm(bool editing, std::function<void(HWND)> action) {
    std::exception_ptr failure;
    formDriver = [&](HWND window) {
        try { action(window); }
        catch (...) { failure = std::current_exception(); DestroyWindow(window); }
        if (IsWindow(window)) DestroyWindow(window); // Ensures a failed check cannot hang the modal loop.
    };
    command(editing ? 22 : 21);
    formDriver = nullptr;
    if (failure) std::rethrow_exception(failure);
}
void setField(HWND window, int field, const wchar_t* value) {
    HWND edit = GetDlgItem(window, 200 + field);
    SetWindowTextW(edit, value);
    SendMessageW(edit, EM_SETMODIFY, TRUE, 0); // Equivalent modified flag to a user edit.
}
void fillForm(HWND window, const wchar_t* id = L"UI-1", const wchar_t* name = L"Ada Student") {
    const wchar_t* values[] = {id, name, L"Computer Science", L"3", L"ada@example.com", L"9.125", L"97.25"};
    for (int i = 0; i < 7; ++i) setField(window, i, values[i]);
}
int runDesktopTests(const std::filesystem::path& root) {
    std::ostringstream report;
    int result = 0;
    try {
        desktopCheck(registry->all().empty() && ListView_GetItemCount(list) == 0, "empty startup");
        desktopCheck(emptyMarkup().find(L"Add your first student") != std::wstring::npos, "native empty-directory message");
        desktopCheck(!IsWindowEnabled(editButton) && !IsWindowEnabled(deleteButton), "no selection disables edit and delete");
        desktopCheck(!shown(undoButton), "undo hidden before deletion");
        driveForm(false, [](HWND window) {
            SendMessageW(window, WM_COMMAND, IDCANCEL, 0);
            desktopCheck(!IsWindow(window), "untouched add form closes without warning");
        });
        driveForm(false, [](HWND window) {
            fillForm(window); setField(window, 3, L"2.5"); lastError.clear();
            SendMessageW(window, WM_COMMAND, IDOK, 0);
            desktopCheck(IsWindow(window) && lastError.find("whole number") != std::string::npos, "invalid semester preserves form");
            desktopCheck(registry->all().empty(), "invalid form does not save");
            setField(window, 3, L"3"); setField(window, 4, L"a@bad..com");
            SendMessageW(window, WM_COMMAND, IDOK, 0);
            desktopCheck(IsWindow(window) && lastError.find("domain") != std::string::npos, "invalid email rejected through desktop");
            setField(window, 4, L"ada@example.com");
            SendMessageW(window, WM_COMMAND, IDOK, 0);
            desktopCheck(!IsWindow(window), "corrected form saves and closes");
        });
        desktopCheck(registry->all().size() == 1 && selectedID == "UI-1", "saved student remains selected");
        desktopCheck(IsWindowEnabled(editButton) && IsWindowEnabled(deleteButton), "selection enables actions");
        desktopCheck(ListView_GetItemCount(list) == 1, "native list no longer in empty state after add");
        driveForm(false, [](HWND window) {
            fillForm(window, L"ui-1"); lastError.clear(); SendMessageW(window, WM_COMMAND, IDOK, 0);
            desktopCheck(IsWindow(window) && lastError.find("already exists") != std::string::npos, "case-insensitive duplicate rejected by form");
        });
        driveForm(true, [](HWND window) {
            desktopCheck((GetWindowLongPtrW(GetDlgItem(window, 200), GWL_STYLE) & ES_READONLY) != 0, "student ID is read only");
            setField(window, 1, L"Ada Updated"); SendMessageW(window, WM_COMMAND, IDOK, 0);
        });
        desktopCheck(registry->find("UI-1")->name() == "Ada Updated", "desktop edit saves");
        desktopCheck(registry->find("UI-1")->cgpa() == 9.125 && registry->find("UI-1")->attendance() == 97.25, "unchanged numeric values retain full precision");
        driveForm(true, [](HWND window) {
            setField(window, 5, L"9.12"); setField(window, 6, L"97.2"); SendMessageW(window, WM_COMMAND, IDOK, 0);
        });
        desktopCheck(registry->find("UI-1")->cgpa() == 9.12 && registry->find("UI-1")->attendance() == 97.2, "explicit edits to rounded numeric text are honored");
        driveForm(true, [](HWND window) {
            setField(window, 1, L"Unsaved name"); testConfirmation = false;
            SendMessageW(window, WM_CLOSE, 0, 0);
            desktopCheck(IsWindow(window), "declining discard keeps editing");
            testConfirmation = true; SendMessageW(window, WM_CLOSE, 0, 0);
            desktopCheck(!IsWindow(window), "confirmed discard closes form");
        });
        desktopCheck(registry->find("UI-1")->name() == "Ada Updated", "discarded edit leaves saved record unchanged");
        SetWindowTextW(searchBox, L"no-such-student");
        desktopCheck(ListView_GetItemCount(list) == 0 && emptyMarkup().find(L"No matching") != std::wstring::npos && selectedID.empty(), "no-match search clears selection and provides native guidance");
        desktopCheck(!IsWindowEnabled(editButton), "filtered-out selection disables edit");
        SetWindowTextW(searchBox, L"COMPUTER");
        desktopCheck(visible.size() == 1, "live case-insensitive course search");
        SetWindowTextW(searchBox, L"\x00e9");
        desktopCheck(visible.empty(), "unsupported search text handled safely");
        SetWindowTextW(searchBox, L"");
        registry->add(Student("UI-2", "B Student", "Arts", 2, "b@example.com", 4.5, 90));
        registry->add(Student("UI-3", "C Student", "Science", 10, "c@example.com", 8, 85)); refresh();
        filter(1); desktopCheck(visible.size() == 1 && visible[0].id() == "UI-2", "needs-support filter");
        filter(2); desktopCheck(visible.size() == 1 && visible[0].id() == "UI-1", "distinction filter");
        filter(3); desktopCheck(visible.size() == 1 && visible[0].id() == "UI-3", "on-track filter");
        filter(0); sortBy(3);
        desktopCheck(visible.front().semester() == 2 && visible.back().semester() == 10, "numeric semester sort");
        sortBy(4); desktopCheck(visible.front().cgpa() == 4.5, "ascending CGPA sort");
        sortBy(4); desktopCheck(visible.front().cgpa() == 9.12, "descending CGPA sort");
        selectRow(0); auto id = selectedID; sortBy(0);
        desktopCheck(selectedID == id, "sorting preserves selection by ID");
        testConfirmation = false; command(23);
        desktopCheck(registry->all().size() == 3, "cancelled delete preserves data");
        testConfirmation = true; command(23);
        desktopCheck(registry->all().size() == 2 && !registry->find(id), "confirmed desktop delete");
        desktopCheck(lastDeleted && shown(undoButton), "undo offered after deletion");
        command(25);
        desktopCheck(registry->all().size() == 3 && registry->find(id) && selectedID == id, "undo restores and selects deleted student");
        desktopCheck(!lastDeleted && !shown(undoButton), "undo consumed after restoration");
        command(43); desktopCheck(page == 2 && !shown(list) && !shown(searchBox), "insights navigation hides directory controls");
        desktopCheck(shown(exportButton), "CSV export remains available on insights");
        command(42); desktopCheck(page == 1 && shown(list), "directory navigation restores controls");
        lastInformation.clear(); command(24);
        int files = 0;
        for (const auto& entry : std::filesystem::directory_iterator(root / "exports")) if (entry.path().extension() == ".csv") ++files;
        desktopCheck(files == 1 && lastInformation.find("Export saved") != std::string::npos, "desktop CSV command creates export");
        Registry other(registry->path()); other.add(Student("UI-4", "External Student", "Arts", 1, "external@example.com", 8, 80));
        lastError.clear(); testConfirmation = true; command(23);
        desktopCheck(lastError.find("Records changed") != std::string::npos && Registry(registry->path()).all().size() == 4, "stale desktop save blocked without data loss");
        command(26); desktopCheck(registry->all().size() == 4 && visible.size() == 4, "reload command reads external changes");
        Registry reloaded(registry->path()); desktopCheck(reloaded.find("UI-1")->name() == "Ada Updated", "desktop saves survive reload");
        for (const auto dimensions : {std::pair<int,int>{1250,850}, {1360,910}, {1600,1000}}) {
            SendMessageW(mainWindow, WM_SIZE, SIZE_RESTORED, MAKELPARAM(dimensions.first, dimensions.second));
            RECT table, edit, search, profile;
            GetWindowRect(list, &table); GetWindowRect(editButton, &edit); GetWindowRect(searchBox, &search); GetClientRect(mainWindow, &profile);
            desktopCheck(table.right < edit.left && search.right < table.right && table.bottom > table.top, "layout avoids control overlap at supported size");
        }
        filter(1); SetWindowTextW(searchBox, L"B Student");
        desktopCheck(visible.size() == 1, "combined standing and search filters");
        driveForm(false, [](HWND window) {
            fillForm(window, L"UI-5", L"New Distinction Student"); SendMessageW(window, WM_COMMAND, IDOK, 0);
        });
        desktopCheck(visible.size() == 5 && textOf(searchBox).empty() && SendMessageW(filterBox, CB_GETCURSEL, 0, 0) == 0, "successful add clears filters so new student is visible");
        desktopCheck(selectedID == "UI-5", "new student stays selected after clearing search and filter");
        driveForm(false, [](HWND window) {
            fillForm(window, L"UNSAVED"); testConfirmation = false; SendMessageW(window, WM_COMMAND, IDCANCEL, 0);
            desktopCheck(IsWindow(window), "dirty add form asks before discarding");
            testConfirmation = true; SendMessageW(window, WM_COMMAND, IDCANCEL, 0);
        });
        desktopCheck(registry->all().size() == 5 && !registry->find("UNSAVED"), "cancelled add never reaches storage");
        driveForm(true, [](HWND window) {
            setField(window, 5, L"nan"); lastError.clear(); SendMessageW(window, WM_COMMAND, IDOK, 0);
            desktopCheck(IsWindow(window) && !lastError.empty(), "non-finite desktop CGPA rejected");
            setField(window, 5, L"10.1"); SendMessageW(window, WM_COMMAND, IDOK, 0);
            desktopCheck(IsWindow(window) && lastError.find("between 0 and 10") != std::string::npos, "out-of-range desktop CGPA rejected");
            setField(window, 5, L"8"); setField(window, 6, L"-1"); SendMessageW(window, WM_COMMAND, IDOK, 0);
            desktopCheck(IsWindow(window) && lastError.find("Attendance") != std::string::npos, "out-of-range desktop attendance rejected");
        });
        // Repeated rebuilds exercise selection notifications and repaint bookkeeping.
        for (int i = 0; i < 100; ++i) { filter(i % 4); SetWindowTextW(searchBox, i % 2 ? L"Student" : L""); }
        filter(0); SetWindowTextW(searchBox, L"");
        desktopCheck(visible.size() == 5 && ListView_GetItemCount(list) == 5, "100 filter/search cycles keep table and vector synchronized");
        command(43);
        desktopCheck(shown(exportButton) && !shown(list) && !shown(editButton), "insights remains usable after repeated filtering");
        command(41);
        command(44);
        desktopCheck(page == 3 && shown(shelfList) && shown(sourceButton) && shown(copyQuoteButton), "reading shelf exposes its list and source actions");
        desktopCheck(!shown(list) && !shown(searchBox) && !shown(editButton), "reading shelf keeps directory controls out of the way");
        desktopCheck(SendMessageW(shelfList, LB_GETCOUNT, 0, 0) == 12, "all twelve sourced passages are listed");
        const auto recordsBeforeReading = registry->all().size();
        for (std::size_t i = 0; i < scholar::literaryQuotes().size(); ++i) {
            SendMessageW(shelfList, LB_SETCURSEL, i, 0);
            SendMessageW(mainWindow, WM_COMMAND, MAKEWPARAM(50, LBN_SELCHANGE), reinterpret_cast<LPARAM>(shelfList));
            desktopCheck(quoteIndex == i, "shelf selection updates the displayed quotation");
            command(47); const auto& q = scholar::literaryQuotes()[i];
            desktopCheck(lastCopied.find(wide(q.text)) != std::wstring::npos && lastCopied.find(wide(q.author)) != std::wstring::npos && lastCopied.find(wide(q.url)) != std::wstring::npos,
                         "copy action retains exact passage, author and source");
            command(46); desktopCheck(lastOpenedSource == wide(q.url), "read original selects the correct fixed source URL");
        }
        command(45); desktopCheck(quoteIndex == 0, "another-passage action wraps to the first entry");
        desktopCheck(registry->all().size() == recordsBeforeReading, "reading and copying never change student records");
        driveForm(false, [](HWND window) { fillForm(window, L"FROM-SHELF", L"Shelf Test Student"); SendMessageW(window, WM_COMMAND, IDOK, 0); });
        desktopCheck(page == 1 && selectedID == "FROM-SHELF" && shown(list), "saving from another page opens the directory and selects the student");
        command(41);
        report << "PASS: " << desktopChecks << " desktop integration checks.\n";
    } catch (const std::exception& e) { report << e.what() << "\nCompleted checks: " << desktopChecks << '\n'; result = 1; }
    std::ofstream log(root / "desktop-results.txt"); log << report.str(); log.close();
    // A deterministic result file in the test executable's folder is read by test.ps1.
    wchar_t executable[32768]; GetModuleFileNameW(nullptr, executable, 32768);
    std::ofstream summary(std::filesystem::path(executable).parent_path() / "desktop-test-results.txt");
    summary << report.str() << "Isolated test data: " << root.string() << '\n'; summary.close();
    DestroyWindow(mainWindow);
    return result;
}
