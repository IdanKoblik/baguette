# Contributing to baguette

Thanks for your interest in contributing! This document describes how to
build the project, the code conventions we follow, and how to get your
changes merged.

## Getting Started

baguette is a Wayland HUD written in **C17**. You'll need:

- A C compiler (`cc` / `gcc` / `clang`)
- `make`
- `pkg-config`
- `wayland-client` and `cairo` development packages
- `wayland-scanner` (to generate protocol code)

Optional, for tooling targets:

- `bear` — generate `compile_commands.json` for clangd (`make compdb`)
- `lcov` — generate an HTML coverage report (`make coverage`)

### Build & Run

```sh
make          # build the `baguette` binary
make run      # build and run
make test     # build and run the unit test suite
make coverage # run tests under coverage and emit an HTML report
make clean     # remove build artifacts and generated protocol code
```

Generated Wayland protocol headers/sources live under
`src/wayland/protocols/` and are produced from the XML in `protocols/` by
`wayland-scanner`. They are regenerated automatically by the build; do not
edit them by hand.

## Code Conventions

Match the surrounding code. The key rules:

- **Language:** C17 (`-std=c17`). Code must compile cleanly with
  `-Wall -Wextra`; do not introduce new warnings.
- **Indentation:** 4 spaces, no tabs (except in the `Makefile`, where
  tabs are required). An `.editorconfig` is provided — please use an
  editor that respects it.
- **Braces:** K&R style — opening brace on the same line.

  ```c
  if (!state) {
      ERROR("Cannot init hud state -> null.");
      return -1;
  }
  ```

- **Naming:**
  - `snake_case` for functions, variables, struct names, and struct
    fields (e.g. `hud_state`, `buffer_compute_geometry`).
  - Functions are prefixed with their module/subject, e.g.
    `hud_state_init`, `draw_hud`, `init_buffer`.
  - `UPPER_CASE` for macros and compile-time constants
    (e.g. `HEIGHT`, `HUD_PADDING`, `NAMESPACE`).
- **Headers:** every header starts with `#pragma once` (we do **not**
  use include guards). Keep includes minimal and ordered: project
  headers, then system/library headers as appropriate.
- **Error handling:** validate inputs (especially pointers) and return
  `-1` / `NULL` on failure. Log the reason with the logging macros rather
  than printing directly:

  ```c
  if (!registry) {
      ERROR("Cannot init hud_state, registry -> null.");
      return -1;
  }
  ```

- **Logging:** use the macros in `src/util/log.h` —
  `INFO`, `WARN`, `ERROR`, `DEBUG`. `ERROR` automatically appends `errno`
  and its string. Don't call `fprintf(stderr, ...)` directly.
- **Comments:** brief `//` comments to explain non-obvious intent, units
  (e.g. `// px`, `// bytes per row`), or assumptions. Don't over-comment
  obvious code.

## Testing

Unit tests use the single-header [greatest](https://github.com/silentbicycle/greatest)
framework (vendored at `tests/greatest.h`).

- **When to add tests:** add or update tests for any new pure/logic
  function or bug fix where behavior can be exercised without a live
  Wayland compositor (e.g. geometry math, buffer/draw helpers, logging).
  Code that only does Wayland I/O is exempt; logic that can be isolated
  should be tested.
- **How to add a test:**
  1. Add `TEST` functions and group them in a `SUITE(...)` inside a
     `tests/test_<area>.c` file (see `tests/test_log.c` for a template).
  2. Declare the suite with `SUITE_EXTERN(<name>_suite);` and register it
     with `RUN_SUITE(<name>_suite);` in `tests/runner.c`.
- **Run them:** `make test` (or `make coverage` for a coverage report).
  All tests must pass before a PR is merged.

## Commits & Pull Requests

- We follow [Conventional Commits](https://www.conventionalcommits.org/):
  `feat:`, `fix:`, `chore:`, `docs:`, etc. — optionally with a scope,
  e.g. `chore(build): Update Makefile`.
- Keep commits focused and the history readable.
- Open your PR against `main` and fill out the pull request template.
- Before submitting, make sure:
  - `make` builds with no new warnings,
  - `make test` passes,
  - your code follows the conventions above.

## Reporting Bugs & Requesting Features

Use the issue templates (Bug report / Feature request). For security
issues, **do not** open a public issue — see [SECURITY.md](SECURITY.md).

Thanks for contributing!
