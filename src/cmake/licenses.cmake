function(ouasim_install_license_bundle destination)
    install(FILES "${PROJECT_SOURCE_DIR}/../LICENSE" "${PROJECT_SOURCE_DIR}/../THIRD_PARTY_NOTICES.md"
        DESTINATION "${destination}" COMPONENT licenses)
    install(DIRECTORY "${PROJECT_SOURCE_DIR}/../licenses/"
        DESTINATION "${destination}/third-party" COMPONENT licenses)
endfunction()
