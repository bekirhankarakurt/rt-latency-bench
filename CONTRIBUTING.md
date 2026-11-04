# Contributing

## Build and test

```bash
cmake -S . -B build -DRTLB_WERROR=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Before opening a pull request, also run the sanitizer build and the analysers:

```bash
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DRTLB_SANITIZE=ON -DRTLB_WERROR=ON
cmake --build build-asan -j && ctest --test-dir build-asan --output-on-failure
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
clang-tidy -p build --quiet src/*.cpp apps/*.cpp
cppcheck --enable=warning,style,performance,portability --error-exitcode=1 --inline-suppr \
  --suppressions-list=.cppcheck-suppressions --std=c++17 -I include --quiet src apps tests
```

## Style

- Modern C++17, RAII, no raw `new`/`delete`, `const` wherever possible, `[[nodiscard]]` on queries, `enum class`.
- The build must be warning-free with `-Wall -Wextra -Wpedantic -Wshadow -Wconversion`.
- No allocation or blocking calls in the measurement loop.
- Anything that needs hardware or privileges must degrade gracefully and be testable through `--simulate` or `--replay`.
- Tests are dependency-free (`tests/test.hpp`); add a `tests/test_<topic>.cpp` file and it is picked up automatically.

## Commits

Small, logical commits with plain imperative subjects, for example `Add replay loader` or
`Fix percentile rank rounding`. Describe the why in the body when it is not obvious.
Update `CHANGELOG.md` for user-visible changes.
