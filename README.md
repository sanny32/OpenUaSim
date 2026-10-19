# OpenUaSim

[![Build and Test](https://github.com/sanny32/OpenUaSim/actions/workflows/build-test.yml/badge.svg)](https://github.com/sanny32/OpenUaSim/actions/workflows/build-test.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

OpenUaSim is an open source OPC UA simulation server with a desktop UI, built on
Qt 6 and [QUaServer](https://github.com/sanny32/QUaServer).

## Features

- Start, stop and restart the OPC UA server from the toolbar or the **Server**
  menu; the status page and status bar show the endpoint, uptime, node count and
  connected clients.
- Build the address space under the Objects folder from folders, objects and
  variables; duplicate, copy and paste whole subtrees, delete several nodes at
  once, rename any node and edit a variable's data type, access level,
  initial value, quality and simulation. Forcing a Bad or Uncertain quality
  tests how clients handle it. Clients read and write the variables and call
  methods added to folders and objects: Reset restores the variables next to
  it to their initial values, SetValue(name, value) sets one of them.
- Drive variables with sine, square, triangle, ramp, random and step waveforms
  with configurable interval, period, amplitude and offset, toggle Booleans,
  count up, or cycle through a list of values such as machine states; values
  are converted to the variable's built-in data type. Array variables of up
  to 1000 elements run the simulation with the phase spread across elements.
- Serve `opc.tcp` endpoints with a choice of security policy and mode, and accept
  anonymous logins or the username/password logins listed in **Server → User
  Accounts**. Set the application name, URI and
  session limit in **Server → Server Settings**.
- Manage certificates in the built-in PKI store (own, trusted, rejected,
  issuers): install the server certificate with its key, import, trust, reject,
  export and delete certificates. Client certificates the server rejects land in
  the rejected folder for review.
- Keep the simulation in `.ouasim` JSON projects — New, Open, Save and Save As;
  the last project reopens on startup and unsaved changes are confirmed before
  they are dropped.
- Import NodeSet2 companion models, browse their objects and variables on the
  Objects page, and remove them again from the Namespaces page. Export the
  project nodes, or any other served namespace, as NodeSet2.
- Inspect the standard types in use, the namespace array, active and closed
  sessions, and the server log filtered by level and category; export the
  filtered log to CSV.
- Switch between light and dark application themes, or follow the system.

### Limitations

- QUaServer serves a single listener, so all enabled endpoints share one TCP
  port; endpoint changes on a running server apply on restart.
- Secure endpoints need an installed server certificate.
- Nodes of imported models are read-only on the Objects page, and their values
  there refresh only when the address space is rebuilt. Removing a model
  restarts a running server, which disconnects its clients.

## Quick start

On first launch OpenUaSim opens a sample project: a `FactoryLine_01` folder with
a motor, a conveyor, a pump and a tank, each holding simulated variables, and an
anonymous unsecured endpoint.

1. Choose **Server → Start Server**.
2. Connect any OPC UA client — for example
   [OpenUaExplorer](https://github.com/sanny32/OpenUaExplorer) — to
   `opc.tcp://localhost:48400` with security `None` and anonymous login.
3. Browse `Objects/FactoryLine_01` and subscribe to a variable to watch it change.

## System requirements

The application runs on:

- <img src="docs/icons/logo_windows.svg" width="16" height="16" /> **Windows** 10 or later (x64)
- <img src="docs/icons/logo_apple.svg" width="16" height="16" /> **macOS** 13 or later (Apple silicon)
- <img src="docs/icons/logo_linux.svg" width="16" height="16" /> **Linux** — any recent x86-64 distribution with glibc 2.28 or newer

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

The helper scripts prepare the dependencies and build a Release version with
Ninja, following the same approach as OpenUaExplorer:

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

Alternatively, configure and build with CMake, pointing `CMAKE_PREFIX_PATH` at your Qt kit:

```sh
cmake -S src -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.9/<kit>
cmake --build build --parallel
```

Options:

| Option                   | Default  | Description                                 |
|--------------------------|----------|---------------------------------------------|
| `OUASIM_BUILD_TESTS`     | `OFF`    | Build the Qt Test suites (needs Qt Test).   |
| `OUASIM_WITH_ENCRYPTION` | `ON`     | Build QUaServer with OpenSSL 3 encryption.  |
| `OUASIM_QUASERVER_TAG`   | `master` | QUaServer git tag or commit to fetch.       |

### Tests

```sh
cmake -S src -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.9/<kit> -DOUASIM_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Feedback

Found a bug or missing a feature? Open an issue in the
[issue tracker](https://github.com/sanny32/OpenUaSim/issues).
Pull requests are welcome.

## MIT License

Copyright (c) 2026 Alexandr Ananev

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
