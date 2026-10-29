install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
ouasim_install_license_bundle("${CMAKE_INSTALL_DATADIR}/licenses/${PROJECT_NAME}")
install(FILES "${PROJECT_SOURCE_DIR}/../.github/linux/usr/share/applications/io.github.sanny32.OpenUaSim.desktop"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
install(FILES "${PROJECT_SOURCE_DIR}/app/res/icons/app.svg"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps"
    RENAME io.github.sanny32.OpenUaSim.svg)
