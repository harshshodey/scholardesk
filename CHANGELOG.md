# Version 1.2

- Added 12 brief, verified human-authored literary passages from eight authors, with original source links, locations and context.
- Replaced promotional screen slogans with attributed excerpts, while retaining clear operational instructions.
- Added a reading shelf with keyboard navigation, exact text, context, Read original and Copy with credit actions.
- Added a sidebar passage browser and contextual passages in forms, search guidance, profile prompts and export confirmation.
- Added console menu option 9 for the same source catalogue.
- Newly saved students now open in the directory even when added from the reading shelf or insights.
- Expanded desktop tests to cover every passage's selection, copied credit and source action, without opening external websites or modifying the user's clipboard during automated checks.
- Kept the existing record format and data unchanged. Literary content is never written into student records or CSV exports.

# Version 1.1

## Data safety and correctness

- Prevented stale windows from silently overwriting each other's saved records. Saves use an OS writer lock and compare the current file to the version originally loaded.
- Added safe reload: F5 on desktop, option 8 in the console. Invalid reloads preserve the existing in-memory directory.
- Corrected numeric editing: an explicitly retyped CGPA or attendance value is saved even when it equals the previously rounded display. Untouched fields retain their full stored precision.
- Protected pre-existing temporary directories when a save cannot start.
- Exported CSV through a temporary file so an unfinished write is not presented as a completed CSV. Existing exports and unfinished export files are protected.
- Tightened email checks for repeated dots, malformed domain labels and invalid characters. Plus-tagged email addresses remain supported.
- Fixed image-list cleanup when the desktop window closes.

## Easier daily use

- Added Undo delete for the most recently deleted student within the current session.
- Added sortable directory columns with sort-direction indicators and numeric sorting.
- Added the On track filter.
- Kept student selection stable when sorting, filtering, or saving a form. Newly saved students are visible even if the previous search/filter would have hidden them.
- Added visible native no-results guidance and an empty-directory message.
- Added confirmation before discarding unsaved form edits, with No as the safe default.
- Focused invalid fields for quick correction and added Ctrl+A support to input fields.
- Made sidebar items focusable native buttons and added keyboard shortcuts for navigation, search, editing, saving/exporting and reloading.
- Fixed the sidebar highlight when changing pages through keyboard shortcuts.
- Kept Export CSV available on Academic insights.
- Preserved the original colour palette, dashboard, student profile and overall layout.

## Verification

Expanded from 54 checks to 151: 85 core checks, 56 desktop integration checks and 10 console integration checks. The desktop harness exercises the real controls, modal form loop and command handlers against isolated temporary data, without changing the supplied directory. See VALIDATION.md for the final run results and limits.
