# Belder Test Suite

This directory contains a GoogleTest-based integration test suite for the `belder` build system.

## Requirements

### Required
- **GNU Make** (for `make test`)
- **g++** with C++23 support (for building both belder and the test runner)
- **GoogleTest** library (`libgtest-dev` on Debian/Ubuntu)
- **GCC/binutils** (`gcc`, `ar`) for C and library tests

### Optional (tests requiring these tools are skipped with full install instructions)
- `clang++` — compiler-switching tests (`--CXX clang++`)
- `clang` — C compiler-switching tests (`--CC clang`)
- `as` / `nasm` — ASM assembler tests

### Install GoogleTest (Debian/Ubuntu)
```bash
sudo apt-get install libgtest-dev
```

On some systems you may also need to build it:
```bash
cd /usr/src/gtest && sudo cmake . && sudo make && sudo cp *.a /usr/lib/
```

## Running the Tests

### Build and run all tests
```bash
make test
```

### Run with verbose output and timing
```bash
make test-verbose
```

### Stop on first failure
```bash
make test-failfast
```

### Pass custom gtest flags via `TEST_ARGS`
```bash
# Run a single test
make test TEST_ARGS="--gtest_filter=BelderFixture.BasicCppBuild"

# Run a subset of tests (glob)
make test TEST_ARGS="--gtest_filter=BelderFixture.*Link*"

# Stop at first failure
make test TEST_ARGS="--gtest_fail_fast"
```

### Run test binary directly
```bash
# After 'make test' has built the binary:
./tests/test_runner --gtest_filter=BelderFixture.BasicCppBuild
./tests/test_runner --gtest_filter='BelderFixture.*Compiler*'
./tests/test_runner --gtest_list_tests
```

### Clean test artifacts
```bash
make mrproper
```

## Diagnostic Output

The test suite produces **maximally detailed failure messages**. When any test
fails, the output includes:

```
============================================================
  Description : -C absolute path: build should succeed when running from /tmp with -C /tmp/btest_abc123
  Command     : ./belder -C /tmp/btest_abc123
  CWD         : /tmp/btest_abc123
  Exit code   : 1
------------------------------------------------------------
  STDOUT:
  Compiling main.cpp
  ...
------------------------------------------------------------
  STDERR:
  error: no such file or directory
============================================================
```

Every `EXPECT_BELDER_OK` / `ASSERT_BELDER_OK` assertion prints:
- **Description** — what the test is verifying
- **Command** — the exact command string that was executed
- **CWD** — the working directory in which it ran
- **Exit code** — the process exit status
- **STDOUT** — full standard output
- **STDERR** — full standard error

This makes failing tests self-explanatory without needing to re-run manually.

## Skipped Tests — Missing Toolchains

When a test requires a tool that is not installed, it is automatically skipped
with **full actionable instructions**. Example:

```
============================================================
SKIPPED: Switch C++ compiler to clang++ via --CXX flag
------------------------------------------------------------
Missing tool(s): clang++

HOW TO INSTALL:
  Debian/Ubuntu : sudo apt-get install clang
  Fedora/RHEL   : sudo dnf install clang
  Arch Linux    : sudo pacman -S clang

VERIFY INSTALLATION:
  which clang++ && clang++ --version

RERUN THIS TEST:
  ./tests/test_runner --gtest_filter=BelderFixture.CXXCompilerSwitch
============================================================
```

The following tests are currently skipped when `clang++`/`clang` are absent:

| Test | Required tools |
|------|---------------|
| `BelderFixture.CXXCompilerSwitch` | `g++`, `clang++` |
| `BelderFixture.CCCompilerSwitch` | `gcc`, `clang` |
| `BelderFixture.CXXCompilerSwitchUsedInLog` | `clang++` |

### Installing missing compilers

**Debian/Ubuntu:**
```bash
sudo apt-get install clang g++ gcc
```

**Fedora/RHEL:**
```bash
sudo dnf install clang gcc-c++ gcc
```

**Arch Linux:**
```bash
sudo pacman -S clang gcc
```

After installation, verify:
```bash
which clang++ && clang++ --version
which clang   && clang   --version
```

Then rerun the previously skipped tests:
```bash
./tests/test_runner --gtest_filter='BelderFixture.CXXCompilerSwitch:BelderFixture.CCCompilerSwitch:BelderFixture.CXXCompilerSwitchUsedInLog'
```

## How Tests Work

Each test uses a `BelderFixture` that:
1. Creates a **unique temporary directory** under `/tmp/btest_XXXXXX`
2. Runs `belder -C <tmpdir>` so all build state is isolated to that directory
3. Captures stdout and stderr using `fork`/`exec` with pipes — no temp files
4. Tears down the temp directory and belder state after the test

This ensures tests are:
- **Self-contained**: no shared state between tests
- **Deterministic**: each test starts with a clean project
- **Non-destructive**: no changes to the system outside `/tmp`

The `~/builder` directory (belder's state store) accumulates per-project entries
during testing. These are keyed by the temp directory path and are automatically
cleaned up by `TearDown`.

## Test Organization

| File | Description |
|------|-------------|
| `test_basic.cpp` | Core build scenarios: C, C++, ASM, library builds, log format, -C flag |
| `test_flags.cpp` | CLI options: `-o`, `--rebuild`, `--relink`, `run`, `config`, incremental builds |
| `test_includes.cpp` | Include path resolution: relative, nested, complex, ambiguous, absolute |
| `test_link.cpp` | Link set options: `--no-link-force`, `--link-force`, `--default-link`, `-l` |
| `test_include_dirs.cpp` | Directory search options: `-I`, `--no-include`, `--defult-include` |
| `test_other_flags.cpp` | Compiler/standard switching, flag clearing, `--compile-flags`, `--link-flags` |

## Known Behavioral Notes

The test suite documents the actual behavior of belder, including some current limitations:

1. **Compilation failure does not stop linking**: belder does not check g++ exit codes. If a file fails to compile, the link step is still attempted (and typically fails). Tests reflect this by checking for error output rather than non-zero exit codes.

2. **`-o -flag` is a soft error**: When `-o` is followed by a flag (e.g., `-o --foo`), belder prints "no file name after -o flag" but continues building.

3. **`--no-link-force`/`--link-force`/`--default-link` with a flag argument**: The flag is passed through as a compiler option rather than causing an argument error.

4. **Non-existent directories in `--no-include`**: Silently ignored (no error message).

5. **Pure ASM projects**: Projects with only `.s`/`.S`/`.asm` files and no C/C++ file are not supported; belder reports "Cannot find entry file".
