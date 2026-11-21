# ScholarDesk 1.2 — validation

Reviewed and tested on Windows x64 with MinGW-w64 GCC 15.2.0, C++17, `-Wall -Wextra -Wpedantic` and static runtime linking. Desktop, console and test executables compiled without warnings.

## Three final regression passes

Executed `test.ps1 -Repeat 3` after the final code fixes. Every pass used fresh isolated temporary data.

| Suite | Pass 1 | Pass 2 | Pass 3 |
|---|---:|---:|---:|
| Core storage and validation | 85 passed | 85 passed | 85 passed |
| Desktop integration | 98 passed | 98 passed | 98 passed |
| Console integration | 12 passed | 12 passed | 12 passed |
| Total | **195 passed** | **195 passed** | **195 passed** |

**585 successful check executions; no failures in the final three runs.** This is evidence for the tested behaviours, not a guarantee against every possible hardware, operating-system or input condition.

## What was checked

### Storage and validation

CRUD; partial and case-insensitive searches; duplicate prevention; reload persistence; quotes; full numeric precision; min/max values; malformed email and file data; NaN/infinity rejection; previous-revision backups; CSV escaping and formula protection; protection of existing exports; failure cleanup; failed-backup rollback; real Windows writer-lock contention; stale additions and deletions; missing/external data changes; and safe reload after conflicts or corrupt input.

### Desktop integration

The dedicated test build runs the real HWND controls, modal form loop, selection notifications and command handlers against the same Registry used by the shipped app. It checks add/edit forms, invalid-input correction, duplicates, read-only IDs, numeric edits and precision, unsaved-form cancellation, live search, all standing filters, sorting (including numeric columns), stable selection, clear-filter behaviour after saving, safe deletion, Undo delete, CSV export, external-change conflicts, reload, navigation visibility, layout separation at three supported sizes, and 100 repeated filter/search cycles.

The additional literary-interface checks cover all 12 shelf entries, exact copied passage/author/source strings, the source URL selected by every Read original action, cycling back to the first passage, shelf visibility, student-data preservation, and opening the directory after adding a student from the shelf.

Confirmation and information popups are stubbed only in the dedicated test executable, so the suite can choose both confirmation branches without requiring user clicks. Clipboard writes and browser launching are also stubbed there: tests verify the prepared content and exact target URL without replacing the user's clipboard or opening browser tabs. Production builds use the native Windows message boxes, clipboard and default browser.

### Console integration

End-to-end piped-input workflows cover add with invalid-input correction; search after restart; edit and standing changes; CSV menu export; cancelled and confirmed deletion; form cancellation and empty state; EOF during a form; preventing incomplete/cancelled forms from saving; forward/backward pagination; viewing an exact literary passage and original link; and rejecting an invalid shelf selection.

## Visual review

Inspected the compiled Windows dashboard, selected profile, academic insights, keyboard search navigation, cleared search and native no-results guidance in the earlier review. Version 1.2 was additionally inspected for sourced headings, sidebar quotation layout and the reading shelf. Visual review caught and resolved the earlier sidebar-highlight repaint and hidden no-results-message issues. The source and built application reflect the fixes. The supplied student data was not modified by these tests or the visual review.

Literary wording was checked separately against retrieved original editions and archival reproductions; six Carroll/Austen/Jerome passages were additionally matched against locally retrieved source text. Full source notes are in LITERARY_SOURCES.md. Automated feature tests verify application behaviour, not the popularity of a quotation or whether any reader will find it funny.

## Repeat the checks

```powershell
./build.ps1
./test.ps1 -Repeat 3
```

The desktop harness has a 30-second timeout per run. It writes its result to `bin/desktop-test-results.txt` and reports the isolated temporary-data location. Core, desktop and console suites never use the supplied `data/students.sdb`.

## Scope limits

- The original local validation used MinGW-w64 and the provided PowerShell scripts. The open-source publication additionally runs CMake with Visual Studio on Windows and GCC on Linux in GitHub Actions; current results are linked from the repository README. macOS has not been exercised.
- Visual checks covered the available display configuration; automated layout checks covered three window sizes. Every possible monitor, DPI setting and accessibility tool was not exercised.
- Writer locks coordinate ScholarDesk instances. Arbitrary editors that ignore those locks can still race with a save; external changes already present when saving are detected. Reload and retry is supported; automatic merging is not.
- Power loss, a physically failing drive, full-disk conditions and very large datasets were not simulated.
- Text input remains printable English/ASCII, as documented.
