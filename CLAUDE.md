# CLAUDE.md

## Standing requirements for Windows software in this repository

Apply these on every task:

1. **CI/CD**: keep the project buildable on GitHub Actions.
   Cross-compile with MSVC on `windows-latest` runners
   (workflow: `.github/workflows/build.yml`).
2. **English UI**: every user-facing string (buttons, message boxes,
   window titles, etc.) must be written in English.
3. **Target architectures are x86 and arm64**: the build must succeed
   with both `-A Win32` and `-A ARM64`, and the CI matrix must include
   both.
4. **English documentation**: all documentation (README.md, CLAUDE.md,
   and any other docs) must be written in English.
