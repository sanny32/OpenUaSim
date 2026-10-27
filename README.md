# OpenUaSim

[![Build and Test](https://github.com/sanny32/OpenUaSim/actions/workflows/build-test.yml/badge.svg)](https://github.com/sanny32/OpenUaSim/actions/workflows/build-test.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

OpenUaSim is an open source OPC UA simulation server with a desktop UI, built on
Qt 6 and [QUaServer](https://github.com/sanny32/QUaServer).

## Features

- Custom address spaces with folders, objects and typed scalar or array
  variables, configurable access levels, initial values and Good, Bad or
  Uncertain quality for testing client behavior.
- Variable simulation with sine, square, triangle, ramp, random and step
  waveforms, Boolean toggles, counters and value lists, with configurable timing,
  amplitude and offset.
- Callable OPC UA methods to reset variables or set their values.
- Numeric limit alarms and Boolean off-normal alarms, with event notifications and
  client acknowledgement.
- In-memory value history accessible through `HistoryRead`, with configurable
  retention by value count and age.
- NodeSet2 companion model import, namespace export and instantiation of imported
  object types and standard variable types. Supported variables in imported
  models can use simulation, alarms and history.
- Variable icons published to clients as the standard `Icon` property (32×32
  PNG, `ImagePNG`).
- `opc.tcp` endpoints with configurable security policies and message security
  modes, anonymous access, username/password and X.509 user certificate
  authentication, certificate trust validation and a configurable session limit.
- Local Discovery Server (LDS) registration with automatic renewal, allowing
  clients to find the server through `FindServers`.

### Limitations

All enabled endpoints share one TCP port.

## Quick start

On first launch OpenUaSim opens a sample project: a `FactoryLine_01` folder with
a motor, a conveyor, a pump and a tank, each holding simulated variables.
The sample includes endpoints with and without message security; the steps below
use the unsecured endpoint with anonymous access.

1. Choose **Server → Start Server**.
2. Connect any OPC UA client — for example
   [OpenUaExplorer](https://github.com/sanny32/OpenUaExplorer) — to
   `opc.tcp://localhost:48400` with security `None` and anonymous login.
3. Browse `Objects/FactoryLine_01` and subscribe to a variable to watch it change.

## System requirements

The application runs on:

- <img src="docs/icons/logo_windows.svg" width="16" height="16" /> **Windows** 10 or later (x64)
- <img src="docs/icons/logo_apple.svg" width="16" height="16" /> **macOS** 13 or later (Apple silicon)
- <img src="docs/icons/logo_linux.svg" width="16" height="16" /> **Linux** — x86-64, glibc 2.28 or newer

Linux binaries require compatible Qt and OpenSSL libraries. Compatibility with
glibc 2.28 requires a build using a compatible toolchain and dependencies; see
[BUILDING.md](BUILDING.md#linux).

## Building

Requirements:

- **CMake** 3.22 or newer and a **C++17** compiler.
- **Qt** 6.9 or newer (Core, Gui, Network, Widgets, Svg; Test for the test suite).
- **Qt LinguistTools** for the application translations.
- **Git** and **Python 3** — QUaServer and open62541 are fetched and built
  during configuration.
- **OpenSSL 3** for encryption (on by default). On Windows the OpenSSL from the Qt
  installer is found automatically.

The build is continuously tested on:

- **Ubuntu Linux** (latest GitHub runner)
- **Windows** (latest GitHub runner, MSVC)
- **macOS** (latest GitHub runner, Apple silicon)

The helper scripts prepare the dependencies and build a Release version with Ninja:

```powershell
# Windows (PowerShell)
.\build.ps1
.\build.ps1 -Tests
.\build.ps1 -InstallPrefix "D:\Apps\OpenUaSim"
```

```sh
# Linux and macOS (Bash)
bash build.sh
bash build.sh --tests
bash build.sh --install="$HOME/.local"
```

Use `-Help` or `--help` for options. See [BUILDING.md](BUILDING.md) for dependency
installation, build directories and installation details.

Alternatively, configure, build and run the tests with CMake, pointing
`CMAKE_PREFIX_PATH` at your Qt kit. Omit `-DOUASIM_BUILD_TESTS=ON` to build without
tests.

```sh
cmake -S src -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.9/<kit> -DOUASIM_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Options:

| Option                   | Default  | Description                                 |
|--------------------------|----------|---------------------------------------------|
| `OUASIM_BUILD_TESTS`     | `OFF`    | Build the Qt Test suites (needs Qt Test).   |
| `OUASIM_WITH_ENCRYPTION` | `ON`     | Build QUaServer with OpenSSL 3 encryption.  |
| `OUASIM_QUASERVER_TAG`   | `master` | QUaServer git tag or commit to fetch.       |

## Feedback

Found a bug or missing a feature? Open an issue in the
[issue tracker](https://github.com/sanny32/OpenUaSim/issues).
Pull requests are welcome.

## MIT License

Copyright (c) 2026 Alexandr Ananev. See [LICENSE](LICENSE) for the full text.
