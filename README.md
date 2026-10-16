# OpenUaSim

OpenUaSim is an OPC UA simulation server with a desktop UI built on Qt 6.
The server side is based on [QUaServer](https://github.com/sanny32/QUaServer).

> **Status:** the UI and the project structure are in place; every page shows
> sample data from `MockData`. The server logic is not wired in yet.

## Features (UI)

- **Status** — server state, endpoints, uptime and quick statistics.
- **Objects** — address space tree, variables of an object, variable simulation settings.
- **Types** — type hierarchy, subtype definitions, type properties and references.
- **Namespaces** — namespace array and namespace details.
- **Endpoints** — endpoints with their security and user token settings.
- **Certificates** — PKI store folders, certificates and certificate details.
- **Sessions** — active and closed client sessions.
- **Log** — server log filtered by level and category.
- Light and dark themes: follows the system, or is set from the theme button in the toolbar.

## Project layout

```
src/
  app/      executable, entry point and icon resources
  core/     UI-independent code
    data/     plain structs describing the server state (ServerSnapshot)
    models/   Qt item models over that data
    mock/     sample ServerSnapshot used until the server is connected
  ui/       widgets library
    pages/    one page per tab (.ui + class)
    widgets/  reusable widgets, the item delegate and view helpers
    style/    light and dark palettes
  cmake/    CMake helpers (Qt, QUaServer, tests)
  tests/    Qt Test suites run by CTest
```

## Building

Requirements: CMake 3.22+, a C++17 compiler and Qt 6.9 or newer (Core, Gui, Widgets, Svg).

Configure:

```sh
cmake -S src -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/<kit>
```

Build:

```sh
cmake --build build --parallel
```

### Options

| Option                   | Default  | Description                                         |
|--------------------------|----------|-----------------------------------------------------|
| `OUASIM_BUILD_TESTS`     | `OFF`    | Build the Qt Test suites.                           |
| `OUASIM_WITH_QUASERVER`  | `OFF`    | Fetch QUaServer and link it into `ouasim_core`.     |
| `OUASIM_QUASERVER_TAG`   | `master` | QUaServer git tag or commit to fetch.               |

## Tests

```sh
cmake -S src -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/<kit> -DOUASIM_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## License

MIT, see [LICENSE](LICENSE).
