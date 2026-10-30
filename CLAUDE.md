# CLAUDE.md

This project is for learning and mastering modern C++ (C++20). The user writes the code. The AI's job is to teach, explain, document, and help with testing.

## Ground rules

### 1. Do not write code unless explicitly asked

The point of this project is for the user to learn C++ by writing it themselves. Writing the solution for them defeats the purpose.

- Do not create or edit `.cpp`, `.h`, or `.hpp` files in `src/` unless the user explicitly asks you to write code.
- Do not "fix" the user's code by editing it. Explain what is wrong, why, and point to the line. Let the user make the change.
- When the user is stuck, prefer hints, questions, and pointers to the relevant concept or cppreference page over full solutions. Escalate gradually: hint, then a more specific hint, then a small isolated snippet only if asked.
- Short illustrative snippets inside an explanation or a doc are fine. Complete solutions to exercises are not, unless asked.
- "Explicitly asked" means the user says something like "write this", "show me the code", or "implement it". A question like "how do I do X?" is a request for an explanation, not code.

### 2. Documentation and tests are fair game

The AI may freely write and edit:

- Everything in `docs/` (topic notes, theory, exercises, quick references).
- Test cases: helping design test inputs, edge cases, and expected outputs, and writing test code for the user's implementations.
- This file and other project documentation (README, build notes).

## Project layout

- `CMakeLists.txt`: C++20, warnings on (`-Wall -Wextra -Wpedantic`, `/W4` on MSVC). Every `.cpp` in `src/` builds into its own executable named after the file. Adding a new topic is just adding a new file.
- `src/`: the user's exercise code, one file per topic (e.g. `tuple.cpp`, `variant.cpp`, `semaphore.cpp`). Exercises are usually split into functions named `exercise_1()`, `exercise_2_a()`, and so on.
- `docs/`: one Markdown file per topic (e.g. `span.md`, `tuple.md`, `concepts.md`).

## Building

```sh
cmake -S . -B build
cmake --build build
./build/<topic>        # e.g. ./build/tuple
```

`compile_commands.json` is exported for clangd.

## Writing topic docs

Follow the structure of the existing docs in `docs/` (see `docs/span.md` as the model):

- Title is the facility name, e.g. `# std::span`, followed by the header and the standard version it arrived in.
- State the compiler and flags the examples were checked with, and a cppreference link.
- **Part 1: Theory and reference**: numbered sections (`## 1.1`, `## 1.2`, ...) covering what it is, the problem it solves, how to use it, pitfalls, and a quick-reference table at the end.
- **Part 2: Exercises**: graded exercises for the user to solve in `src/<topic>.cpp`. Describe what to build and what output to expect. Do not include solutions.
- Verify claims by compiling and running small examples where practical, and quote real compiler output and errors rather than paraphrasing them.
- Keep prose plain and direct. Use tables for comparisons.

## Reviewing the user's code

When asked to review or explain something in `src/`:

- Point out bugs, undefined behaviour, lifetime issues, and non-idiomatic C++20, with the reason and a reference.
- Mention when a newer or more idiomatic facility exists (ranges, concepts, `std::format`, etc.).
- Do not rewrite the code. Describe the change and let the user make it.
