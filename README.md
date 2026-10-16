# OpenUaSim

OpenUaSim is an OPC UA simulation server with a desktop UI built on Qt 6.
The server side is based on [QUaServer](https://github.com/sanny32/QUaServer).

## Features

- **Server** — start, stop and restart an OPC UA server (QUaServer / open62541) from the toolbar or
  the Server menu; the status page and status bar show the endpoint, uptime, nodes and clients.
- **Objects** — folders, objects and variables under the Objects folder: add and delete nodes, edit a
  variable's name, data type, access level and simulation; clients read and write the variables.
- **Simulation** — sine, square, triangle, ramp, random and step waveforms with interval, period,
  amplitude and offset, converted to the variable's built-in data type.
- **Endpoints** — opc.tcp endpoints with security policy and mode, anonymous and username/password
  logins. All enabled endpoints share one TCP port; changes on a running server apply on restart.
- **Certificates** — PKI store (own, trusted, rejected, issuers) below the application data
  directory: install the server certificate with its key, import, trust, reject, export and delete
  certificates. Secure endpoints need an installed server certificate.
- **Projects** — New, Open, Save and Save As of `.ouasim` JSON projects; the last project reopens on
  startup and unsaved changes are confirmed before they are dropped.
- **Models** — import NodeSet2 companion models and export the project nodes as NodeSet2.
- **Types, Namespaces, Sessions, Log** — standard types in use, the namespace array, active and
  closed sessions, and the server log filtered by level and category.
- Light and dark themes: follows the system, or is set from the theme button in the toolbar.

### Limitations

- QUaServer serves one listener, so endpoints cannot use different ports.
- Imported NodeSet2 models stay loaded until the application restarts; their nodes are served but
  not listed on the Objects page.
- Rejected client certificates are not collected automatically.

## Project layout

```
src/
  app/      executable, entry point and icon resources
  core/     UI-independent code
    data/        plain structs describing the project and the server state
    models/      Qt item models over that data
    project/     project file, document, node tree helpers, NodeSet2 export
    server/      SimServer, address space, controller, snapshot for the UI
    simulation/  waveforms, data type conversion, simulation engine
    pki/         certificate store
  ui/       widgets library
    pages/    one page per tab (.ui + class)
    dialogs/  dialogs (.ui + class)
    widgets/  reusable widgets, the item delegate and view helpers
    style/    light and dark palettes
  cmake/    CMake helpers (Qt, QUaServer, tests)
  tests/    Qt Test suites run by CTest, sample data and test certificates
```

## Building

Requirements: CMake 3.22+, a C++17 compiler, Qt 6.9 or newer (Core, Gui, Network, Widgets, Svg),
Git and Python 3. QUaServer and open62541 are fetched and built during configuration;
encryption (on by default) needs OpenSSL 3; on Windows the one from the Qt installer is found automatically.

Configure:

```sh
cmake -S src -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/<kit>
```

Build:

```sh
cmake --build build --parallel
```

### Options

| Option                   | Default         | Description                                       |
|--------------------------|-----------------|---------------------------------------------------|
| `OUASIM_BUILD_TESTS`     | `OFF`           | Build the Qt Test suites (needs Qt Test).          |
| `OUASIM_WITH_ENCRYPTION` | `ON`            | Build QUaServer with OpenSSL 3 encryption.        |
| `OUASIM_QUASERVER_TAG`   | pinned commit   | QUaServer git tag or commit to fetch.             |

## Tests

```sh
cmake -S src -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/<kit> -DOUASIM_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## License

MIT, see [LICENSE](LICENSE).
