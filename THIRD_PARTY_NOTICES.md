# Third-party notices

OpenUaSim is licensed under the MIT License in `LICENSE`. Binary packages include
the following dependencies; their license texts are in `licenses/third-party`
inside the installed package (and in `licenses/` in the source tree).

| Component | License | Source |
|-----------|---------|--------|
| Qt Core, Gui, Network, Widgets, SVG and runtime plugins | LGPL-3.0, with GPL-3.0 text included as required by LGPL | https://code.qt.io/cgit/qt/ |
| QUaServer | MIT | https://github.com/sanny32/QUaServer |
| open62541 | MPL-2.0 and the included third-party notices | https://github.com/open62541/open62541 |
| OpenSSL 3 | Apache-2.0 | https://github.com/openssl/openssl |
| Lucide icons | ISC and MIT | https://github.com/lucide-icons/lucide |

Qt and OpenSSL are dynamically linked. Their bundled libraries can be replaced
with compatible builds. QUaServer and open62541 are linked into the application;
their source versions are selected in `src/cmake/quaserver.cmake` and QUaServer's
`cmake/Dependencies.cmake`. Qt binaries and OpenSSL sources are obtained through
the Qt installer, with versions selected in the packaging workflows.
