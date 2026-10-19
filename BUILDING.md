# Building OpenUaSim

`build.ps1` (Windows) and `build.sh` (Linux/macOS) prepare the build dependencies,
configure `src` with CMake and Ninja, and build Release binaries. Minimum CMake
and Qt versions are read from the project's CMake files. QUaServer and open62541
are fetched by CMake; Git and network access are needed on the first build.

## Windows

```powershell
.\build.ps1
.\build.ps1 -Tests
.\build.ps1 -Install
.\build.ps1 -InstallPrefix "D:\Apps\OpenUaSim"
```

The script uses an installed MSVC Qt 6 kit under `C:\Qt`, preferring the newest
version that meets the project minimum and includes LinguistTools. Otherwise,
it downloads a suitable final Qt release with aqtinstall. Qt SVG is included in
the standard Qt download; Qt Charts and Qt OPC UA are not needed.

Python 3.9+, CMake and Visual Studio C++ Build Tools are checked first. If one is
missing, the script offers installation through winget or Chocolatey. Ninja and
OpenSSL 3 are reused from Qt Tools or downloaded with aqtinstall. Visual Studio's
x64 developer environment is loaded automatically. Git must be installed.

Build output is in `build-ouasim-Qt_<version>_msvc_x86_64-Release`.
`-Install` defaults to `C:/Program Files/OpenUaSim`; choose a writable prefix or
run with the permissions required by that directory.

## Linux

```sh
bash build.sh
bash build.sh --tests
bash build.sh --install
bash build.sh --install="$HOME/.local"
```

The script supports Debian/Ubuntu/Astra, Fedora/RHEL, ALT Linux, openSUSE and
Arch families. It installs missing development packages through the distribution
package manager, using sudo when required, and prefers a suitable distribution Qt.
When Qt is too old or unavailable it downloads Qt using aqtinstall into
`.build-tools/qt`. Python tools and a newer CMake, when needed, use a local virtual
environment in `.build-tools`. Older systems can build a local Python 3.12.

The Qt binary selection accounts for the host glibc: glibc 2.28 systems use
Qt 6.9 rather than the Qt 6.10+ binaries requiring glibc 2.34. To produce binaries
compatible with glibc 2.28, build on a glibc 2.28 system with compatible dependencies.
Building on a newer distribution does not provide that compatibility automatically.

If the distribution lacks OpenSSL 3, the script downloads the sources supplied
by Qt and builds them with `tools/build_openssl.sh`. `OPENSSL_ROOT_DIR` can select
an existing OpenSSL installation. `CC` and `CXX` can select compilers.

Build output is in `build-ouasim-Qt_<version>_gcc_<architecture>-Release`.
Without an explicit installation prefix, system Qt builds install into
`/usr/local`; downloaded Qt builds install into `<build-directory>/install`.

## macOS

The Bash options are the same as on Linux. Install Xcode Command Line Tools
and Homebrew first. The script installs or updates CMake, Ninja, Qt, OpenSSL 3,
Git and Python through Homebrew.

Build output is in `build-ouasim-Qt_<version>_clang_<architecture>-Release`.
The default installation prefix is `$HOME/Applications`.

## Tests and installation

`-Tests` / `--tests` enables `OUASIM_BUILD_TESTS`, builds the test executables and
runs the OpenUaSim CTest suites with output on failure. A failed configuration,
build or test run stops the script before installation.

`-InstallPrefix` / `--install=PREFIX` implies installation. Installation uses the
project's existing CMake install rules. It does not create a standalone package:
Qt and other runtime libraries must remain available on the target system.

For manual CMake commands and project options, see [README.md](README.md#building).
