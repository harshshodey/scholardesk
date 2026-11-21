# Contributing to ScholarDesk

Bug reports, usability improvements, documentation corrections and focused pull requests are welcome.

## Build and test

With a C++17 MinGW-w64 compiler and windres in PATH, run from the repository root on Windows:

```powershell
./build.ps1
./test.ps1
```

The tests use isolated temporary records. Do not test with real student information or commit personal records, exports, credentials or build outputs.

For a portable CMake build:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

On Linux and macOS, CMake builds the console edition and core tests. The desktop edition is Windows-only. See README.md for executable locations.

## Pull requests

- Explain the concrete problem and the resulting behaviour.
- Keep changes focused and preserve the existing file format unless a deliberate migration is provided.
- Add meaningful coverage when changing persistence, validation or desktop workflows.
- Include a screenshot when changing visual layout.
- Keep literary wording sourced and attributed; mark excerpts and preserve the original context notes.
- Check that no real student data, tokens, local paths or generated binaries appear in your diff.

By contributing original material, you agree to license it under the project's MIT license. Third-party literary material keeps its own rights and attribution; see THIRD_PARTY_NOTICES.md.
