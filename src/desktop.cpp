// Native Windows interface. Shares the same tested C++ model and file storage as the console.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shellapi.h>
#include "registry.hpp"
#include "literature.hpp"
#include <chrono>
#include <functional>
#include <fstream>
#include <exception>
#include <iomanip>
#include <memory>
#include <sstream>

using scholar::Student;
using scholar::Registry;
namespace {
constexpr COLORREF navy = RGB(17, 29, 45), background = RGB(245, 247, 251), ink = RGB(28, 43, 62),
    muted = RGB(116, 129, 147), teal = RGB(15, 125, 114), line = RGB(230, 235, 242), white = RGB(255, 255, 255);
HINSTANCE instance;
HFONT normalFont, smallFont, titleFont, metricFont, boldFont, logoFont;
HIMAGELIST rowImages = nullptr;
HWND mainWindow, list, searchBox, filterBox, addButton, editButton, deleteButton, exportButton, undoButton, navigation[4];
HWND shelfList, sourceButton, copyQuoteButton, nextQuoteButton;
std::size_t quoteIndex = 5;
std::unique_ptr<Registry> registry;
std::vector<Student> visible;
std::string selectedID, notice = "Everything is up to date. Changes save automatically.";
int page = 0;
int width = 1320, height = 850;
bool refreshing = false;
int sortColumn = 0;
bool sortAscending = true;
std::optional<Student> lastDeleted;
#ifdef SCHOLAR_DESKTOP_TESTS
std::function<void(HWND)> formDriver;
std::string lastError, lastInformation;
std::wstring lastCopied, lastOpenedSource;
bool testConfirmation = false;
int runDesktopTests(const std::filesystem::path& root);
#endif
std::wstring wide(const std::string& s) { return std::wstring(s.begin(), s.end()); }
std::string narrow(const std::wstring& s) {
    std::string out;
    for (wchar_t c : s) {
        if (c < 32 || c >= 127) throw std::invalid_argument("Please use printable English characters.");
        out += static_cast<char>(c);
    }
    return out;
}
std::wstring textOf(HWND control) {
    auto length = GetWindowTextLengthW(control);
    std::wstring text(static_cast<std::size_t>(length) + 1, L'\0');
    GetWindowTextW(control, text.data(), length + 1); text.resize(length); return text;
}
std::wstring quoteText(std::size_t index) { return wide(scholar::literaryDisplay(index)); }
void copyLiteraryQuote() {
    const auto& q = scholar::literaryQuotes().at(quoteIndex);
    const auto value = quoteText(quoteIndex) + L"\r\n" + wide(scholar::literaryCredit(quoteIndex)) + L"\r\n" + wide(q.url);
#ifdef SCHOLAR_DESKTOP_TESTS
    lastCopied = value;
#else
    const auto bytes = (value.size() + 1) * sizeof(wchar_t);
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!memory) throw std::runtime_error("Could not prepare text for the clipboard.");
    auto* buffer = GlobalLock(memory);
    if (!buffer) { GlobalFree(memory); throw std::runtime_error("Could not prepare the clipboard text."); }
    memcpy(buffer, value.c_str(), bytes); GlobalUnlock(memory);
    if (!OpenClipboard(mainWindow)) { GlobalFree(memory); throw std::runtime_error("Clipboard is busy. Please try again."); }
    if (!EmptyClipboard() || !SetClipboardData(CF_UNICODETEXT, memory)) {
        CloseClipboard(); GlobalFree(memory); throw std::runtime_error("Could not copy the quotation.");
    }
    CloseClipboard(); // Windows now owns memory.
#endif
    notice = "Quotation copied with its author, work and source link.";
    InvalidateRect(mainWindow, nullptr, FALSE);
}
std::string decimal(double value, int places = 1) {
    std::ostringstream out; out << std::fixed << std::setprecision(places) << value; return out.str();
}
double parse(const std::string& text, const std::string& label) {
    std::istringstream in(scholar::trim(text)); double value;
    if (!(in >> value)) throw std::invalid_argument(label + ": enter a number.");
    in >> std::ws;
    if (!in.eof() || !std::isfinite(value)) throw std::invalid_argument(label + ": enter a finite number.");
    return value;
}
void fill(HDC dc, RECT rect, COLORREF color) {
    auto brush = CreateSolidBrush(color); FillRect(dc, &rect, brush); DeleteObject(brush);
}
void card(HDC dc, RECT rect, COLORREF color = white, int radius = 18) {
    auto brush = CreateSolidBrush(color); auto pen = CreatePen(PS_SOLID, 1, color);
    auto oldBrush = SelectObject(dc, brush); auto oldPen = SelectObject(dc, pen);
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
    SelectObject(dc, oldBrush); SelectObject(dc, oldPen); DeleteObject(brush); DeleteObject(pen);
}
void label(HDC dc, int x, int y, int w, int h, const std::wstring& text, HFONT font = nullptr, COLORREF color = ink,
           UINT flags = DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS | DT_VCENTER) {
    auto old = SelectObject(dc, font ? font : normalFont); SetTextColor(dc, color); SetBkMode(dc, TRANSPARENT);
    RECT area{x, y, x + w, y + h}; DrawTextW(dc, text.c_str(), -1, &area, flags); SelectObject(dc, old);
}
HWND control(HWND parent, const wchar_t* cls, const wchar_t* text, DWORD style, int id) {
    HWND result = CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style, 0, 0, 10, 10,
                                 parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance, nullptr);
    SendMessageW(result, WM_SETFONT, reinterpret_cast<WPARAM>(normalFont), TRUE); return result;
}
void error(HWND owner, const std::exception& e) {
#ifdef SCHOLAR_DESKTOP_TESTS
    (void)owner; lastError = e.what();
#else
    MessageBoxW(owner, wide(e.what()).c_str(), L"Please check your entry", MB_OK | MB_ICONWARNING);
#endif
}
bool confirm(HWND owner, const std::wstring& message, const wchar_t* title) {
#ifdef SCHOLAR_DESKTOP_TESTS
    (void)owner; (void)message; (void)title; return testConfirmation;
#else
    return MessageBoxW(owner, message.c_str(), title, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDYES;
#endif
}
void information(HWND owner, const std::wstring& message, const wchar_t* title) {
#ifdef SCHOLAR_DESKTOP_TESTS
    (void)owner; (void)title; lastInformation = std::string(message.begin(), message.end());
#else
    MessageBoxW(owner, message.c_str(), title, MB_OK | MB_ICONINFORMATION);
#endif
}
LRESULT CALLBACK editKeys(HWND window, UINT message, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR) {
    if (message == WM_KEYDOWN && wp == 'A' && GetKeyState(VK_CONTROL) < 0) {
        SendMessageW(window, EM_SETSEL, 0, -1); return 0;
    }
    if (message == WM_NCDESTROY) RemoveWindowSubclass(window, editKeys, 1);
    return DefSubclassProc(window, message, wp, lp);
}
void selectCurrent() {
    int row = ListView_GetNextItem(list, -1, LVNI_SELECTED);
    selectedID = row >= 0 && static_cast<std::size_t>(row) < visible.size() ? visible[row].id() : "";
    EnableWindow(editButton, !selectedID.empty()); EnableWindow(deleteButton, !selectedID.empty());
    InvalidateRect(mainWindow, nullptr, FALSE);
}
void refresh() {
    std::string query;
    auto value = textOf(searchBox);
    // Non-ASCII search yields no match instead of crashing the interface.
    try { query = narrow(value); } catch (...) { query = "\x01"; }
    visible = registry->search(query);
    int filter = static_cast<int>(SendMessageW(filterBox, CB_GETCURSEL, 0, 0));
    visible.erase(std::remove_if(visible.begin(), visible.end(), [&](const Student& s) {
        return (filter == 1 && s.standing() != "Needs support") || (filter == 2 && s.standing() != "Distinction") ||
            (filter == 3 && s.standing() != "On track");
    }), visible.end());
    std::sort(visible.begin(), visible.end(), [](const Student& a, const Student& b) {
        int order = 0;
        switch (sortColumn) {
        case 1: order = scholar::lower(a.id()).compare(scholar::lower(b.id())); break;
        case 2: order = scholar::lower(a.course()).compare(scholar::lower(b.course())); break;
        case 3: order = (a.semester() > b.semester()) - (a.semester() < b.semester()); break;
        case 4: order = (a.cgpa() > b.cgpa()) - (a.cgpa() < b.cgpa()); break;
        case 5: order = (a.attendance() > b.attendance()) - (a.attendance() < b.attendance()); break;
        case 6: order = a.standing().compare(b.standing()); break;
        default: order = scholar::lower(a.name()).compare(scholar::lower(b.name()));
        }
        if (order == 0) order = scholar::lower(a.id()).compare(scholar::lower(b.id()));
        return sortAscending ? order < 0 : order > 0;
    });
    const auto previous = selectedID;
    refreshing = true;
    SendMessageW(list, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(list);
    for (std::size_t i = 0; i < visible.size(); ++i) {
        const auto& s = visible[i];
        auto name = wide(s.name());
        LVITEMW item{}; item.mask = LVIF_TEXT; item.iItem = static_cast<int>(i); item.pszText = name.data();
        ListView_InsertItem(list, &item);
        std::vector<std::wstring> cells{wide(s.id()), wide(s.course()), std::to_wstring(s.semester()),
            wide(decimal(s.cgpa(), 2)), wide(decimal(s.attendance()) + "%"), wide(s.standing())};
        for (int c = 0; c < static_cast<int>(cells.size()); ++c) ListView_SetItemText(list, item.iItem, c + 1, cells[c].data());
        if (s.id() == previous) ListView_SetItemState(list, item.iItem, LVIS_SELECTED, LVIS_SELECTED);
    }
    refreshing = false;
    auto header = ListView_GetHeader(list);
    for (int i = 0; i < 7; ++i) {
        HDITEMW item{}; item.mask = HDI_FORMAT; Header_GetItem(header, i, &item);
        item.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
        if (i == sortColumn) item.fmt |= sortAscending ? HDF_SORTUP : HDF_SORTDOWN;
        Header_SetItem(header, i, &item);
    }
    SendMessageW(list, WM_SETREDRAW, TRUE, 0); InvalidateRect(list, nullptr, TRUE);
    ShowWindow(undoButton, lastDeleted ? SW_SHOW : SW_HIDE);
    selectCurrent();
}
void layout() {
    const int content = width - 280;
    MoveWindow(addButton, width - 193, 35, 155, 42, TRUE);
    MoveWindow(searchBox, 284, 315, std::max(160, content - 568), 34, TRUE);
    MoveWindow(filterBox, width - 553, 315, 191, 180, TRUE);
    MoveWindow(exportButton, width - 324, 35, 110, 42, TRUE);
    MoveWindow(undoButton, width - 455, 35, 119, 42, TRUE);
    for (int i = 0; i < 4; ++i) {
        MoveWindow(navigation[i], 18, 158 + i * 57, 202, 47, TRUE); InvalidateRect(navigation[i], nullptr, TRUE);
    }
    const int tableWidth = width - 620;
    MoveWindow(list, 278, 372, tableWidth, std::max(100, height - 455), TRUE);
    const int panelX = width - 320;
    MoveWindow(editButton, panelX + 18, height - 150, 116, 38, TRUE);
    MoveWindow(deleteButton, panelX + 144, height - 150, 116, 38, TRUE);
    int usable = tableWidth - 20;
    int columns[] = {23, 14, 19, 7, 9, 12, 16};
    for (int i = 0; i < 7; ++i) ListView_SetColumnWidth(list, i, usable * columns[i] / 100);
    const bool directory = page < 2;
    ShowWindow(list, directory ? SW_SHOW : SW_HIDE);
    ShowWindow(searchBox, directory ? SW_SHOW : SW_HIDE);
    ShowWindow(filterBox, directory ? SW_SHOW : SW_HIDE);
    ShowWindow(editButton, directory ? SW_SHOW : SW_HIDE);
    ShowWindow(deleteButton, directory ? SW_SHOW : SW_HIDE);
    MoveWindow(shelfList, 298, 219, 310, std::max(150, height - 338), TRUE);
    MoveWindow(sourceButton, 646, height - 169, 144, 40, TRUE);
    MoveWindow(copyQuoteButton, 804, height - 169, 144, 40, TRUE);
    MoveWindow(nextQuoteButton, 34, height - 124, 168, 28, TRUE);
    ShowWindow(shelfList, page == 3 ? SW_SHOW : SW_HIDE);
    ShowWindow(sourceButton, page == 3 ? SW_SHOW : SW_HIDE);
    ShowWindow(copyQuoteButton, page == 3 ? SW_SHOW : SW_HIDE);
    InvalidateRect(mainWindow, nullptr, FALSE);
}
void paintMain(HDC dc) {
    fill(dc, {0, 0, width, height}, background); fill(dc, {0, 0, 238, height}, navy);
    card(dc, {26, 33, 65, 72}, RGB(105, 223, 193), 12);
    label(dc, 32, 35, 30, 35, L"S", logoFont, navy);
    label(dc, 78, 30, 154, 30, L"ScholarDesk", logoFont, white);
    label(dc, 79, 58, 145, 19, L"CAMPUS WORKSPACE", smallFont, RGB(142, 161, 183));
    label(dc, 30, 122, 170, 20, L"MANAGE", smallFont, RGB(116, 139, 164));
    card(dc, {21, height - 274, 217, height - 82}, RGB(27, 45, 62), 14);
    label(dc, 36, height - 261, 165, 20, L"FROM THE ORIGINALS", smallFont, RGB(143, 163, 184));
    label(dc, 36, height - 231, 161, 86, quoteText(quoteIndex), smallFont, RGB(209, 224, 237), DT_LEFT | DT_WORDBREAK);
    label(dc, 36, height - 148, 165, 19, wide(scholar::literaryQuotes()[quoteIndex].author), smallFont, RGB(120, 231, 204));
    label(dc, 30, height - 55, 190, 20, L"VERSION 1.2   /   C++17", smallFont, RGB(111, 136, 159));
    label(dc, 278, 27, 500, 18, L"WORKSPACE   /   STUDENTS", smallFont, muted);
    const std::size_t titleIndex = page == 1 ? 3 : page == 2 ? 8 : 0;
    label(dc, 276, 56, width - 540, 45, page == 3 ? L"The reading shelf" : page == 2 ? L"To strive, to seek, to find" : quoteText(titleIndex), page == 2 ? logoFont : titleFont);
    label(dc, 278, 108, width - 350, 24, page == 3 ? quoteText(7) + L" - Jane Austen" :
        wide(scholar::literaryCredit(titleIndex)) + (page == 0 ? L"  /  Overview" : page == 1 ? L"  /  Student directory" : L"  /  Academic insights; title excerpt"), smallFont, muted);
    if (page == 3) {
        card(dc, {278, 157, width - 38, height - 80});
        label(dc, 300, 172, 300, 28, L"12 PASSAGES / 8 AUTHORS", smallFont, muted);
        const auto& q = scholar::literaryQuotes()[quoteIndex];
        label(dc, 646, 182, width - 718, 24, q.fragment ? L"VERIFIED EXCERPT" : L"VERIFIED PASSAGE", smallFont, teal);
        label(dc, 644, 226, width - 712, 148, quoteText(quoteIndex), logoFont, ink, DT_LEFT | DT_WORDBREAK);
        label(dc, 646, 392, width - 714, 28, wide(q.author), logoFont, teal);
        label(dc, 646, 430, width - 714, 43, wide(q.work), normalFont, ink, DT_LEFT | DT_WORDBREAK);
        label(dc, 646, 481, width - 714, 42, wide(q.locator), smallFont, muted, DT_LEFT | DT_WORDBREAK);
        label(dc, 646, 537, width - 714, std::max(45, height - 720), wide(q.context), smallFont, muted, DT_LEFT | DT_WORDBREAK);
        label(dc, 302, height - 113, width - 365, 22, L"Author credits are retained when copied. Read original opens the linked edition in your browser.", smallFont, muted);
        label(dc, 282, height - 56, width - 590, 28, wide(notice), smallFont, teal);
        label(dc, width - 282, height - 56, 245, 28, L"CTRL+4  /  READING SHELF", smallFont, muted, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
        return;
    }
    const auto& all = registry->all(); double average = 0, attendance = 0; int support = 0, distinction = 0;
    for (const auto& s : all) { average += s.cgpa(); attendance += s.attendance(); support += s.standing() == "Needs support"; distinction += s.standing() == "Distinction"; }
    int cw = (width - 314) / 3;
    const wchar_t* names[] = {L"TOTAL STUDENTS", L"AVERAGE CGPA", L"NEEDS SUPPORT"};
    std::wstring values[] = {std::to_wstring(all.size()), all.empty() ? L"--" : wide(decimal(average / all.size(), 2)), std::to_wstring(support)};
    const wchar_t* hints[] = {L"Student records, saved locally", L"On a 10-point academic scale", L"Attendance <75% or CGPA <5"};
    for (int i = 0; i < 3; ++i) {
        int x = 278 + i * (cw + 12);
        card(dc, {x, 157, x + cw, 272});
        label(dc, x + 20, 171, cw - 32, 22, names[i], smallFont, muted);
        label(dc, x + 19, 198, cw - 28, 42, values[i], metricFont, i == 1 ? teal : ink);
        label(dc, x + 20, 243, cw - 28, 18, hints[i], smallFont, muted);
    }
    if (page == 2) {
        card(dc, {278, 296, width - 38, height - 80});
        label(dc, 302, 314, 550, 32, L"Academic snapshot", logoFont);
        label(dc, 302, 365, 700, 30, L"Average attendance", normalFont, muted);
        label(dc, 302, 396, 300, 40, all.empty() ? L"--" : wide(decimal(attendance / all.size()) + "%"), metricFont, teal);
        label(dc, width - 418, 365, 340, 30, L"Students with distinction", normalFont, muted);
        label(dc, width - 418, 396, 300, 40, std::to_wstring(distinction), metricFont);
        label(dc, 302, 461, 600, 28, L"COURSE DISTRIBUTION", smallFont, muted);
        std::vector<std::pair<std::string, int>> courses;
        for (const auto& s : all) {
            auto found = std::find_if(courses.begin(), courses.end(), [&](const auto& c) { return c.first == s.course(); });
            if (found == courses.end()) courses.emplace_back(s.course(), 1); else ++found->second;
        }
        std::sort(courses.begin(), courses.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
        int y = 500, shown = 0;
        for (const auto& c : courses) {
            if (y + 34 > height - 130) break;
            label(dc, 302, y, 245, 23, wide(c.first), normalFont);
            card(dc, {560, y + 7, width - 130, y + 17}, line, 8);
            card(dc, {560, y + 7, 560 + static_cast<int>((width - 690) * static_cast<double>(c.second) / all.size()), y + 17}, RGB(68, 178, 157), 8);
            label(dc, width - 112, y, 50, 23, std::to_wstring(c.second), boldFont, teal);
            y += 37; ++shown;
        }
        label(dc, 302, height - 120, width - 370, 24, wide(std::to_string(courses.size()) + " courses total  |  Showing top " + std::to_string(shown) + ". Distinction: CGPA >=8.5 and attendance >=75%."), smallFont, muted);
    } else {
        card(dc, {278, 296, width - 342, height - 80});
        // Native controls sit within this card; their accessible text stays standard Windows UI.
        label(dc, 296, height - 112, width - 650, 24, wide(std::to_string(visible.size()) + " students shown   /   Select a row to view details. Double-click to edit."), smallFont, muted);
        int x = width - 320;
        card(dc, {x, 296, width - 38, height - 80});
        label(dc, x + 20, 313, 242, 25, L"STUDENT PROFILE", smallFont, muted);
        const auto* s = registry->find(selectedID);
        if (s) {
            std::wstring initials;
            initials += static_cast<wchar_t>(std::toupper(static_cast<unsigned char>(s->name()[0])));
            auto space = s->name().find_last_of(' ');
            if (space != std::string::npos && space + 1 < s->name().size()) initials += static_cast<wchar_t>(std::toupper(static_cast<unsigned char>(s->name()[space + 1])));
            card(dc, {x + 20, 355, x + 78, 413}, RGB(226, 245, 239), 18);
            label(dc, x + 20, 355, 58, 58, initials, logoFont, teal, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            label(dc, x + 20, 414, 242, 30, wide(s->name()), logoFont);
            label(dc, x + 20, 448, 242, 22, wide(s->id()), normalFont, muted);
            const std::vector<std::pair<std::wstring, std::wstring>> details{{L"Course / Semester", wide(s->course() + " / " + std::to_string(s->semester()))},
                {L"Email", wide(s->email())}, {L"CGPA / Attendance", wide(decimal(s->cgpa(), 2) + "  /  " + decimal(s->attendance()) + "%")}};
            int y = 480;
            for (const auto& pair : details) {
                label(dc, x + 20, y, 242, 17, pair.first, smallFont, muted);
                label(dc, x + 20, y + 19, 242, 23, pair.second, normalFont); y += 44;
            }
            label(dc, x + 20, height - 191, 242, 25, wide(s->standing()), boldFont, s->standing() == "Needs support" ? RGB(173, 99, 33) : teal);
        } else {
            card(dc, {x + 20, 365, x + 78, 423}, RGB(236, 241, 247), 18);
            label(dc, x + 20, 365, 58, 58, L"S", logoFont, muted, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            label(dc, x + 20, 447, 242, 30, quoteText(3), normalFont);
            label(dc, x + 20, 478, 242, 20, L"Lewis Carroll | Alice in Wonderland", smallFont, teal);
            label(dc, x + 20, 514, 238, 100, L"Select a student from the directory to see their profile, update a record or manage their details.", normalFont, muted, DT_LEFT | DT_WORDBREAK);
        }
    }
    label(dc, 282, height - 56, width - 590, 28, wide(notice), smallFont, teal);
    label(dc, width - 282, height - 56, 245, 28, L"F5 RELOAD  /  CTRL+F SEARCH", smallFont, muted, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
}

struct Form {
    HWND window{}, fields[7]{};
    bool editing = false, closed = false, saved = false;
    Student original;
    std::vector<std::wstring> initial;
};
bool formDirty(const Form& form) {
    for (int i = 0; i < 7; ++i) if (textOf(form.fields[i]) != form.initial[i]) return true;
    return false;
}
LRESULT CALLBACK formProc(HWND window, UINT msg, WPARAM wp, LPARAM lp) {
    auto* form = reinterpret_cast<Form*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        form = static_cast<Form*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(form));
    }
    switch (msg) {
    case WM_CREATE: {
        const wchar_t* labels[] = {L"Student ID", L"Full name", L"Course", L"Semester (1-12)", L"Email address", L"CGPA (0-10)", L"Attendance % (0-100)"};
        std::vector<std::wstring> values(7);
        if (form->editing) {
            const auto& s = form->original;
            values = {wide(s.id()), wide(s.name()), wide(s.course()), std::to_wstring(s.semester()), wide(s.email()), wide(decimal(s.cgpa(), 2)), wide(decimal(s.attendance()))};
        }
        form->initial = values;
        for (int i = 0; i < 7; ++i) {
            int x = i == 6 ? 300 : 36, y = 112 + std::min(i, 5) * 68;
            if (i == 5) y = 452;
            if (i == 6) y = 452;
            HWND caption = control(window, L"STATIC", labels[i], 0, 100 + i);
            MoveWindow(caption, x, y, 244, 22, TRUE);
            form->fields[i] = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", values[i].c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                x, y + 25, (i >= 5 ? 244 : 508), 32, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(200 + i)), instance, nullptr);
            SendMessageW(form->fields[i], WM_SETFONT, reinterpret_cast<WPARAM>(normalFont), TRUE);
            SetWindowSubclass(form->fields[i], editKeys, 1, 0);
            SendMessageW(form->fields[i], EM_SETLIMITTEXT, i == 0 ? 16 : i == 1 ? 60 : i == 2 ? 40 : i == 4 ? 80 : 24, 0);
        }
        if (form->editing) SendMessageW(form->fields[0], EM_SETREADONLY, TRUE, 0);
        HWND cancel = control(window, L"BUTTON", L"Cancel", WS_TABSTOP | BS_OWNERDRAW, IDCANCEL);
        HWND save = control(window, L"BUTTON", L"Save student", WS_TABSTOP | BS_OWNERDRAW, IDOK);
        MoveWindow(cancel, 280, 552, 118, 40, TRUE); MoveWindow(save, 412, 552, 132, 40, TRUE);
        SetFocus(form->fields[form->editing ? 1 : 0]); return 0;
    }
#ifdef SCHOLAR_DESKTOP_TESTS
    case WM_TIMER:
        KillTimer(window, 1);
        if (formDriver) formDriver(window);
        return 0;
#endif
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc = BeginPaint(window, &ps); RECT r; GetClientRect(window, &r); fill(dc, r, white);
        label(dc, 34, 24, 515, 37, quoteText(form->editing ? 2 : 1), logoFont);
        label(dc, 36, 66, 510, 24, L"Lewis Carroll | " + std::wstring(form->editing ? L"Update student" : L"Add student") + L" | All fields required", smallFont, muted);
        label(dc, 36, 518, 508, 23, quoteText(9) + L" - Benjamin Franklin", smallFont, teal);
        EndPaint(window, &ps); return 0;
    }
    case WM_CTLCOLORSTATIC: SetTextColor(reinterpret_cast<HDC>(wp), muted); SetBkColor(reinterpret_cast<HDC>(wp), white); return reinterpret_cast<LRESULT>(GetStockObject(WHITE_BRUSH));
    case WM_COMMAND:
        if (LOWORD(wp) == IDCANCEL) { SendMessageW(window, WM_CLOSE, 0, 0); return 0; }
        if (LOWORD(wp) == IDOK) {
            int invalidField = 0;
            try {
                std::vector<std::string> v;
                for (int i = 0; i < 7; ++i) { invalidField = i; v.push_back(narrow(textOf(form->fields[i]))); }
                invalidField = 0; Student(v[0], "Test", "Course", 1, "a@b.co", 0, 0);
                if (!form->editing && registry->find(v[0])) throw std::invalid_argument("This student ID already exists. Choose a different ID.");
                invalidField = 1; Student(v[0], v[1], "Course", 1, "a@b.co", 0, 0);
                invalidField = 2; Student(v[0], v[1], v[2], 1, "a@b.co", 0, 0);
                invalidField = 3;
                double semester = parse(v[3], "Semester");
                if (semester < 1 || semester > 12 || std::floor(semester) != semester) throw std::invalid_argument("Semester must be a whole number from 1 to 12.");
                invalidField = 4; Student(v[0], v[1], v[2], static_cast<int>(semester), v[4], 0, 0);
                invalidField = 5; double cgpa = parse(v[5], "CGPA");
                Student(v[0], v[1], v[2], static_cast<int>(semester), v[4], cgpa, 0);
                invalidField = 6; double attendance = parse(v[6], "Attendance");
                Student(v[0], v[1], v[2], static_cast<int>(semester), v[4], cgpa, attendance);
                if (form->editing) {
                    if (!SendMessageW(form->fields[5], EM_GETMODIFY, 0, 0)) cgpa = form->original.cgpa();
                    if (!SendMessageW(form->fields[6], EM_GETMODIFY, 0, 0)) attendance = form->original.attendance();
                }
                Student candidate(v[0], v[1], v[2], static_cast<int>(semester), v[4], cgpa, attendance);
                if (form->editing) registry->update(form->original.id(), candidate); else registry->add(candidate);
                selectedID = candidate.id(); notice = candidate.name() + (form->editing ? " - changes saved." : " - added to your directory.");
                form->saved = true; DestroyWindow(window);
            } catch (const std::exception& e) {
                error(window, e); SetFocus(form->fields[invalidField]);
                SendMessageW(form->fields[invalidField], EM_SETSEL, 0, -1);
            }
            return 0;
        }
        break;
    case WM_CLOSE:
        if (!formDirty(*form) || confirm(window, L"Discard your unsaved changes?\n\nChoose No to keep editing.", L"Keep editing?")) DestroyWindow(window);
        return 0;
    case WM_DESTROY: form->closed = true; return 0;
    }
    // Reuse the shared button painter from the main window.
    if (msg == WM_DRAWITEM) return SendMessageW(mainWindow, msg, wp, lp);
    return DefWindowProcW(window, msg, wp, lp);
}
void openForm(bool editing) {
    Form form;
    form.editing = editing;
    if (editing) {
        const auto* student = registry->find(selectedID);
        if (!student) return;
        form.original = *student;
    }
    RECT parent; GetWindowRect(mainWindow, &parent);
    EnableWindow(mainWindow, FALSE);
    form.window = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT, L"ScholarForm", editing ? L"Update student - ScholarDesk" : L"Add student - ScholarDesk",
        WS_CAPTION | WS_SYSMENU | WS_POPUP, parent.left + (parent.right - parent.left - 600) / 2,
        parent.top + (parent.bottom - parent.top - 660) / 2, 596, 654, mainWindow, nullptr, instance, &form);
    if (!form.window) { EnableWindow(mainWindow, TRUE); throw std::runtime_error("Could not open student form."); }
#ifdef SCHOLAR_DESKTOP_TESTS
    SetTimer(form.window, 1, 1, nullptr);
#else
    ShowWindow(form.window, SW_SHOW); SetFocus(form.fields[editing ? 1 : 0]);
#endif
    MSG message;
    while (!form.closed && GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (message.message == WM_KEYDOWN && message.wParam == VK_ESCAPE) { SendMessageW(form.window, WM_CLOSE, 0, 0); continue; }
        if (message.message == WM_KEYDOWN && ((message.wParam == 'S' && GetKeyState(VK_CONTROL) < 0) || message.wParam == VK_RETURN)) {
            SendMessageW(form.window, WM_COMMAND, GetFocus() == GetDlgItem(form.window, IDCANCEL) ? IDCANCEL : IDOK, 0); continue;
        }
        if (!IsDialogMessageW(form.window, &message)) { TranslateMessage(&message); DispatchMessageW(&message); }
    }
    EnableWindow(mainWindow, TRUE);
#ifndef SCHOLAR_DESKTOP_TESTS
    SetForegroundWindow(mainWindow);
#endif
    if (form.saved) {
        refreshing = true; SetWindowTextW(searchBox, L""); SendMessageW(filterBox, CB_SETCURSEL, 0, 0); refreshing = false;
        page = 1; layout();
    }
    refresh();
}
LRESULT CALLBACK mainProc(HWND window, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        mainWindow = window;
        searchBox = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 0,0,10,10,window,reinterpret_cast<HMENU>(11),instance,nullptr);
        SendMessageW(searchBox, WM_SETFONT, reinterpret_cast<WPARAM>(normalFont), TRUE);
        SetWindowSubclass(searchBox, editKeys, 1, 0);
        SendMessageW(searchBox, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"Search name, ID, course or email..."));
        filterBox = control(window, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP, 12);
        for (auto text : {L"All academic standings", L"Needs support", L"Distinction", L"On track"}) SendMessageW(filterBox, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text));
        SendMessageW(filterBox, CB_SETCURSEL, 0, 0);
        addButton = control(window, L"BUTTON", L"+  Add student", BS_OWNERDRAW | WS_TABSTOP, 21);
        editButton = control(window, L"BUTTON", L"Edit profile", BS_OWNERDRAW | WS_TABSTOP, 22);
        deleteButton = control(window, L"BUTTON", L"Delete", BS_OWNERDRAW | WS_TABSTOP, 23);
        exportButton = control(window, L"BUTTON", L"Export CSV", BS_OWNERDRAW | WS_TABSTOP, 24);
        undoButton = control(window, L"BUTTON", L"Undo delete", BS_OWNERDRAW | WS_TABSTOP, 25);
        const wchar_t* navText[] = {L"01    Overview", L"02    Student directory", L"03    Academic insights", L"04    The reading shelf"};
        for (int i = 0; i < 4; ++i) navigation[i] = control(window, L"BUTTON", navText[i], BS_OWNERDRAW | WS_TABSTOP, 41 + i);
        nextQuoteButton = control(window, L"BUTTON", L"Another passage  >", BS_OWNERDRAW | WS_TABSTOP, 45);
        sourceButton = control(window, L"BUTTON", L"Read original", BS_OWNERDRAW | WS_TABSTOP, 46);
        copyQuoteButton = control(window, L"BUTTON", L"Copy with credit", BS_OWNERDRAW | WS_TABSTOP, 47);
        shelfList = control(window, L"LISTBOX", L"Literary passages", WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT, 50);
        for (const auto& q : scholar::literaryQuotes()) {
            const auto entry = wide(std::string(q.author) + " - " + q.text);
            SendMessageW(shelfList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(entry.c_str()));
        }
        SendMessageW(shelfList, LB_SETCURSEL, quoteIndex, 0);
        SendMessageW(shelfList, LB_SETTOPINDEX, 0, 0);
        list = CreateWindowExW(0, WC_LISTVIEWW, L"Student directory", WS_VISIBLE | WS_CHILD | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
            0,0,100,100,window,reinterpret_cast<HMENU>(30),instance,nullptr);
        SendMessageW(list, WM_SETFONT, reinterpret_cast<WPARAM>(normalFont), TRUE);
        ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP);
        ListView_SetBkColor(list, white); ListView_SetTextBkColor(list, white); ListView_SetTextColor(list, ink);
        rowImages = ImageList_Create(1, 44, ILC_COLOR32, 1, 1); ListView_SetImageList(list, rowImages, LVSIL_SMALL);
        const wchar_t* columns[] = {L"Student", L"ID", L"Course", L"Sem", L"CGPA", L"Attend.", L"Standing"};
        for (int i = 0; i < 7; ++i) { LVCOLUMNW column{}; column.mask = LVCF_TEXT | LVCF_WIDTH; column.pszText = const_cast<wchar_t*>(columns[i]); column.cx = 100; ListView_InsertColumn(list, i, &column); }
        refresh(); return 0;
    }
    case WM_GETMINMAXINFO: { auto* info = reinterpret_cast<MINMAXINFO*>(lp); info->ptMinTrackSize = {1250, 850}; return 0; }
    case WM_SIZE: if (wp != SIZE_MINIMIZED) { width = LOWORD(lp); height = HIWORD(lp); layout(); } return 0;
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc = BeginPaint(window, &ps);
        HDC buffer = CreateCompatibleDC(dc); HBITMAP bitmap = CreateCompatibleBitmap(dc, width, height); auto old = SelectObject(buffer, bitmap);
        paintMain(buffer); BitBlt(dc, 0, 0, width, height, buffer, 0, 0, SRCCOPY);
        SelectObject(buffer, old); DeleteObject(bitmap); DeleteDC(buffer); EndPaint(window, &ps); return 0;
    }
    case WM_DRAWITEM: {
        auto* item = reinterpret_cast<DRAWITEMSTRUCT*>(lp); auto r = item->rcItem;
        if (item->CtlID == 50) {
            if (item->itemID >= scholar::literaryQuotes().size()) return TRUE;
            const auto& q = scholar::literaryQuotes()[item->itemID];
            const bool selected = (item->itemState & ODS_SELECTED) != 0;
            fill(item->hDC, r, selected ? RGB(226, 245, 239) : white);
            label(item->hDC, r.left + 10, r.top + 4, r.right-r.left-20, 22, wide(q.author), boldFont, selected ? teal : ink);
            label(item->hDC, r.left + 10, r.top + 26, r.right-r.left-20, 20, wide(q.text), smallFont, muted);
            if (item->itemState & ODS_FOCUS) { InflateRect(&r, -2, -2); DrawFocusRect(item->hDC, &r); }
            return TRUE;
        }
        if (item->CtlID == 45) {
            fill(item->hDC, r, RGB(27, 45, 62));
            label(item->hDC, 0, 0, r.right, r.bottom, textOf(item->hwndItem), smallFont, RGB(120, 231, 204), DT_LEFT | DT_SINGLELINE | DT_VCENTER);
            if (item->itemState & ODS_FOCUS) DrawFocusRect(item->hDC, &r);
            return TRUE;
        }
        if (item->CtlID >= 41 && item->CtlID <= 44) {
            fill(item->hDC, r, navy);
            bool active = page == static_cast<int>(item->CtlID) - 41;
            if (active) card(item->hDC, r, RGB(37, 61, 77), 12);
            label(item->hDC, 15, 0, r.right - 20, r.bottom, textOf(item->hwndItem), normalFont, active ? RGB(120, 231, 204) : RGB(174, 191, 208));
            if (item->itemState & ODS_FOCUS) { InflateRect(&r, -3, -3); DrawFocusRect(item->hDC, &r); }
            return TRUE;
        }
        bool primary = item->CtlID == 21 || item->CtlID == IDOK;
        bool disabled = (item->itemState & ODS_DISABLED) != 0;
        COLORREF bg = primary ? teal : item->CtlID == 23 ? RGB(253, 239, 235) : RGB(234, 241, 244);
        if (item->itemState & ODS_SELECTED) bg = RGB(202, 225, 221);
        fill(item->hDC, r, background); card(item->hDC, r, disabled ? line : bg, 12);
        label(item->hDC, r.left, r.top, r.right-r.left, r.bottom-r.top, textOf(item->hwndItem), boldFont,
            disabled ? muted : primary ? white : item->CtlID == 23 ? RGB(168, 69, 51) : ink, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        if (item->itemState & ODS_FOCUS) { InflateRect(&r, -3, -3); DrawFocusRect(item->hDC, &r); }
        return TRUE;
    }
    case WM_MEASUREITEM:
        if (reinterpret_cast<MEASUREITEMSTRUCT*>(lp)->CtlID == 50) { reinterpret_cast<MEASUREITEMSTRUCT*>(lp)->itemHeight = 52; return TRUE; }
        break;
    case WM_NOTIFY: {
        auto* notification = reinterpret_cast<NMHDR*>(lp);
        if (notification->hwndFrom == list) {
            if (notification->code == LVN_GETEMPTYMARKUP) {
                auto* empty = reinterpret_cast<NMLVEMPTYMARKUP*>(lp);
                empty->dwFlags = EMF_CENTERED;
                lstrcpynW(empty->szMarkup, registry->all().empty() ? L"\"Begin at the beginning\" - Lewis Carroll\nAdd your first student to get started." :
                    L"\"Who in the world am I?\" - Lewis Carroll\nNo matching students. Try another search or standing.", L_MAX_URL_LENGTH);
                return TRUE;
            }
            if (notification->code == LVN_ITEMCHANGED && !refreshing) selectCurrent();
            if (notification->code == LVN_COLUMNCLICK) {
                int column = reinterpret_cast<NMLISTVIEW*>(lp)->iSubItem;
                sortAscending = column == sortColumn ? !sortAscending : true; sortColumn = column; refresh();
            }
            if (notification->code == NM_DBLCLK && !selectedID.empty()) PostMessageW(window, WM_COMMAND, 22, 0);
            if (notification->code == LVN_KEYDOWN && !selectedID.empty()) {
                auto key = reinterpret_cast<NMLVKEYDOWN*>(lp)->wVKey;
                if (key == VK_RETURN) PostMessageW(window, WM_COMMAND, 22, 0);
                if (key == VK_DELETE) PostMessageW(window, WM_COMMAND, 23, 0);
            }
            if (notification->code == NM_CUSTOMDRAW) {
                auto* draw = reinterpret_cast<NMLVCUSTOMDRAW*>(lp);
                if (draw->nmcd.dwDrawStage == CDDS_PREPAINT) return CDRF_NOTIFYITEMDRAW;
                if (draw->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
                    draw->clrTextBk = draw->nmcd.dwItemSpec % 2 ? RGB(249, 251, 253) : white;
                    draw->clrText = ink; return CDRF_NOTIFYSUBITEMDRAW;
                }
                if (draw->nmcd.dwDrawStage == (CDDS_ITEMPREPAINT | CDDS_SUBITEM) && draw->iSubItem == 6 && draw->nmcd.dwItemSpec < visible.size())
                    draw->clrText = visible[draw->nmcd.dwItemSpec].standing() == "Needs support" ? RGB(174, 101, 37) : teal;
                return CDRF_DODEFAULT;
            }
        }
        break;
    }
    case WM_COMMAND:
        try {
            if (!refreshing && ((LOWORD(wp) == 11 && HIWORD(wp) == EN_CHANGE) || (LOWORD(wp) == 12 && HIWORD(wp) == CBN_SELCHANGE))) refresh();
            if (LOWORD(wp) >= 41 && LOWORD(wp) <= 44) {
                page = LOWORD(wp) - 41; layout();
                for (HWND nav : navigation) InvalidateRect(nav, nullptr, TRUE);
            }
            if (LOWORD(wp) == 45) {
                quoteIndex = (quoteIndex + 1) % scholar::literaryQuotes().size();
                SendMessageW(shelfList, LB_SETCURSEL, quoteIndex, 0);
                InvalidateRect(mainWindow, nullptr, FALSE);
            }
            if (LOWORD(wp) == 50 && HIWORD(wp) == LBN_SELCHANGE) {
                auto selected = SendMessageW(shelfList, LB_GETCURSEL, 0, 0);
                if (selected >= 0 && static_cast<std::size_t>(selected) < scholar::literaryQuotes().size()) quoteIndex = static_cast<std::size_t>(selected);
                InvalidateRect(mainWindow, nullptr, FALSE);
            }
            if (LOWORD(wp) == 46) {
                auto url = wide(scholar::literaryQuotes()[quoteIndex].url);
#ifdef SCHOLAR_DESKTOP_TESTS
                lastOpenedSource = url;
#else
                if (reinterpret_cast<INT_PTR>(ShellExecuteW(window, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL)) <= 32)
                    throw std::runtime_error("Could not open the browser. The source link is also included when you copy the quotation.");
#endif
            }
            if (LOWORD(wp) == 47) copyLiteraryQuote();
            if (LOWORD(wp) == 21) openForm(false);
            if (LOWORD(wp) == 22) openForm(true);
            if (LOWORD(wp) == 23 && !selectedID.empty()) {
                const auto id = selectedID; const auto* s = registry->find(id);
                if (s && confirm(window, wide("Delete " + s->name() + " (" + id + ")?\n\nYou can undo this deletion until you close the app or delete another record."), L"Delete student?")) {
                    Student removed = *s;
                    registry->remove(id); lastDeleted = removed; selectedID.clear(); notice = "Student deleted. Use Undo delete to bring this record back."; refresh();
                }
            }
            if (LOWORD(wp) == 25 && lastDeleted) {
                registry->add(*lastDeleted); selectedID = lastDeleted->id(); lastDeleted.reset();
                refreshing = true; SetWindowTextW(searchBox, L""); SendMessageW(filterBox, CB_SETCURSEL, 0, 0); refreshing = false;
                notice = "Deletion undone. The student is back in your directory."; refresh();
            }
            if (LOWORD(wp) == 26) { registry->reload(); notice = "Directory reloaded from disk. Your view is up to date."; refresh(); }
            if (LOWORD(wp) == 24) {
                auto stamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                auto target = registry->path().parent_path().parent_path() / "exports" / ("students-" + std::to_string(stamp) + ".csv");
                registry->exportCsv(target); notice = "CSV exported to " + target.string(); InvalidateRect(window, nullptr, FALSE);
                information(window, wide("Export saved:\n" + std::filesystem::absolute(target).string()) + L"\n\n" + quoteText(6) + L"\n- Oscar Wilde, The Importance of Being Earnest (Act II)", L"Your export is ready");
            }
        } catch (const std::exception& e) { error(window, e); }
        return 0;
    case WM_CTLCOLORSTATIC:
        SetTextColor(reinterpret_cast<HDC>(wp), muted); SetBkColor(reinterpret_cast<HDC>(wp), white);
        return reinterpret_cast<LRESULT>(GetStockObject(WHITE_BRUSH));
    case WM_DESTROY: {
        if (rowImages) { ListView_SetImageList(list, nullptr, LVSIL_SMALL); ImageList_Destroy(rowImages); rowImages = nullptr; }
        PostQuitMessage(0); return 0;
    }
    }
    return DefWindowProcW(window, msg, wp, lp);
}
HFONT font(int size, int weight = FW_NORMAL) { return CreateFontW(-size, 0,0,0,weight,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI"); }
#ifdef SCHOLAR_DESKTOP_TESTS
#include "../tests/desktop_tests.inc"
#endif
}
int WINAPI wWinMain(HINSTANCE app, HINSTANCE, PWSTR, int show) {
    instance = app;
    SetProcessDPIAware();
    INITCOMMONCONTROLSEX common{sizeof(common), ICC_LISTVIEW_CLASSES}; InitCommonControlsEx(&common);
    normalFont = font(15); smallFont = font(12); titleFont = font(30, FW_SEMIBOLD); metricFont = font(33, FW_SEMIBOLD); boldFont = font(14, FW_SEMIBOLD); logoFont = font(20, FW_SEMIBOLD);
    try {
        // Resolve storage beside the executable's parent folder, independent of launch location.
        wchar_t executable[32768]; DWORD length = GetModuleFileNameW(nullptr, executable, 32768);
        if (!length || length >= 32768) throw std::runtime_error("Cannot determine application location.");
        auto root = std::filesystem::path(executable).parent_path();
        for (int depth = 0; depth < 4; ++depth) {
            if (std::filesystem::exists(root / "data") || std::filesystem::exists(root / "CMakeLists.txt")) break;
            root = root.parent_path();
        }
#ifdef SCHOLAR_DESKTOP_TESTS
        root = std::filesystem::temp_directory_path() / ("scholardesk-desktop-" + std::to_string(GetCurrentProcessId()) + "-" + std::to_string(GetTickCount64()));
        std::filesystem::create_directories(root / "data");
#endif
        registry = std::make_unique<Registry>(root / "data" / "students.sdb");
        WNDCLASSW cls{}; cls.hInstance = app; cls.hCursor = LoadCursorW(nullptr, IDC_ARROW); cls.lpfnWndProc = mainProc; cls.lpszClassName = L"ScholarDesk"; cls.hIcon = LoadIconW(nullptr, IDI_APPLICATION); RegisterClassW(&cls);
        cls.lpfnWndProc = formProc; cls.lpszClassName = L"ScholarForm"; cls.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)); RegisterClassW(&cls);
        HWND window = CreateWindowExW(0, L"ScholarDesk", L"ScholarDesk | Student workspace", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
            CW_USEDEFAULT,CW_USEDEFAULT,1360,910,nullptr,nullptr,app,nullptr);
        if (!window) throw std::runtime_error("Cannot create application window.");
#ifdef SCHOLAR_DESKTOP_TESTS
        return runDesktopTests(root);
#endif
        ShowWindow(window, show); UpdateWindow(window);
        MSG msg;
        while (GetMessageW(&msg,nullptr,0,0) > 0) {
            if (msg.message == WM_KEYDOWN) {
                bool ctrl = GetKeyState(VK_CONTROL) < 0;
                int command = 0;
                if (ctrl && msg.wParam == 'F') { page = 1; layout(); SetFocus(searchBox); SendMessageW(searchBox, EM_SETSEL, 0, -1); continue; }
                if (ctrl && msg.wParam == 'N') command = 21;
                if (ctrl && msg.wParam == 'E') command = 22;
                if (ctrl && msg.wParam == 'S') command = 24;
                if (ctrl && msg.wParam >= '1' && msg.wParam <= '4') command = 41 + static_cast<int>(msg.wParam - '1');
                if (msg.wParam == VK_F5) command = 26;
                if (msg.wParam == VK_ESCAPE && page < 2) { SetWindowTextW(searchBox, L""); SendMessageW(filterBox, CB_SETCURSEL, 0, 0); refresh(); continue; }
                if (command) { SendMessageW(window, WM_COMMAND, command, 0); continue; }
            }
            if (!IsDialogMessageW(window, &msg)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
        }
    } catch (const std::exception& e) { MessageBoxW(nullptr, wide(e.what()).c_str(), L"ScholarDesk could not start", MB_OK | MB_ICONERROR); return 1; }
    for (auto f : {normalFont, smallFont, titleFont, metricFont, boldFont, logoFont}) DeleteObject(f);
    return 0;
}
