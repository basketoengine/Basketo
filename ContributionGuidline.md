\
# Contribution Guidelines for Basketo Game Engine

First off, thank you for considering contributing to the Basketo Game Engine! We appreciate your interest and effort. These guidelines will help you get started.

## How to Contribute

We welcome contributions in various forms:

*   **Reporting Bugs:** If you find a bug, please open an issue on our GitHub repository. Describe the bug clearly, including steps to reproduce it, what you expected to happen, and what actually happened. Include your OS, compiler version, and any relevant logs.
*   **Suggesting Enhancements:** If you have an idea for a new feature or an improvement to an existing one, please open an issue to discuss it. This allows us to coordinate efforts and ensure the suggestion aligns with the project's goals.
*   **Pull Requests:** If you'd like to contribute code:
    1.  **Fork the repository** and create your branch from `main` (or the current development branch).
    2.  **Set up your development environment** by following the instructions in the [README.md](README.md).
    3.  **Make your changes.** Ensure your code adheres to the project's coding style (see below).
    4.  **Add/update tests** if your changes involve new functionality or bug fixes that can be tested.
    5.  **Ensure your code builds successfully** on your local machine.
    6.  **Write clear and concise commit messages** (see below).
    7.  **Push your changes** to your fork.
    8.  **Open a pull request** against the main Basketo repository. Provide a clear description of your changes and why they are being made. Link to any relevant issues.

## Coding Standards

While we are still formalizing a strict style guide, please try to adhere to the following general principles:

*   **Consistency:** Try to match the style of the existing codebase.
*   **Readability:** Write clear and understandable code. Use meaningful variable and function names.
*   **Comments:** Comment your code where necessary, especially for complex logic or non-obvious decisions.
*   **C++ Best Practices:** Follow modern C++ (C++17 as per `CMakeLists.txt`) best practices.
*   **Header Files:**
    *   Use `#pragma once` for include guards.
    *   Organize includes (e.g., standard library, external libraries, project headers).

## Commit Message Guidelines

Please follow these conventions for your commit messages:

*   Start with a concise summary of changes (e.g., `Fix: Player collision detection issue`).
*   Use the imperative mood (e.g., `Add feature` not `Added feature` or `Adds feature`).
*   If your commit addresses an issue, reference it in the commit body (e.g., `Fixes #123`).
*   Provide a more detailed explanation in the commit body if the summary isn't sufficient.

Example:

```
Fix: Prevent player from falling through thin platforms

The previous collision detection logic did not adequately handle
continuous collision detection for fast-moving objects, allowing
the player to sometimes pass through thin platforms. This change
implements a more robust swept-AABB check.

Fixes #42
```

## Development Environment

Please refer to the [README.md](README.md) for instructions on how to set up your development environment, including required dependencies and build steps.

## Testing

*   If you are fixing a bug, try to write a test that reproduces the bug before your fix and passes after.
*   If you are adding new functionality, please add tests to cover it.
*   Currently, physics tests can be found in `tests/test_physics.cpp`. New tests can be added there or in new files as appropriate.

## Code of Conduct

Please note that this project is released with a Contributor Code of Conduct. By participating in this project you agree to abide by its terms. (We will add a formal CODE_OF_CONDUCT.md file soon. For now, please be respectful and considerate in all interactions.)

---

Thank you again for your interest in contributing!
