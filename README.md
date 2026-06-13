# Producer-Consumer Simulator

A command-line visualization of the bounded producer-consumer problem for an
Operating Systems course. The simulator uses POSIX threads, counting
semaphores, and mutexes to coordinate producers and consumers sharing a
circular buffer.

## Requirements

- Linux or WSL
- GCC or Clang with C11 support
- POSIX threads and unnamed semaphores
- GNU Make

macOS builds use a condition-variable fallback because Darwin does not support
unnamed POSIX semaphores. Linux builds exercise the required `sem_t`
implementation.

## Build and Run

Open a terminal in the repository root, then compile the project:

```bash
make
```

Start the interactive simulator:

```bash
make run
```

Use the menu to select an action:

```text
1. Run Balanced scenario
2. Run Fast Producer scenario
3. Run Fast Consumer scenario
4. Help
0. Exit
```

Each run prints synchronized event logs followed by production totals, wait
counts, elapsed time, throughput, and correctness checks.

## Run Tests

Build the application and run all automated tests:

```bash
make test
```

The test target covers the circular buffer, scenarios, statistics, logger,
simulation lifecycle, concurrent stress behavior, and scripted CLI flows.
A successful run ends with `All tests passed`.

Generate a coverage report:

```bash
make coverage
```

Coverage requires Clang, `llvm-profdata`, and `llvm-cov`, and enforces at least
80% line coverage. The report is written to
`build/coverage/report.txt`.

Remove compiled objects, test binaries, coverage data, and the executable:

```bash
make clean
```

## Architecture

`main` delegates to the CLI, which selects a scenario and starts the
simulation. The simulation owns thread creation and cleanup. Workers access
the bounded buffer and report through independent logger and statistics
services. Consumers terminate after receiving one sentinel item each.
