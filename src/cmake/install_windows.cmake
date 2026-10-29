if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "C:/Program Files/${PRODUCT_NAME}" CACHE PATH "Install path prefix" FORCE)
endif()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION .)
ouasim_install_license_bundle(licenses)

if(OUASIM_DEPLOY_RUNTIME)
    find_program(OUASIM_DEPLOY_TOOL windeployqt HINTS "${QT_BINARY_DIR}" REQUIRED)
    get_filename_component(OUASIM_OPENSSL_BIN "${OPENSSL_INCLUDE_DIR}/../bin" ABSOLUTE)
    ouasim_install_runtime()
endif()
