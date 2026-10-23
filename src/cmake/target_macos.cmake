function(ouasim_configure_target_macos target_name)
    set(MACOSX_BUNDLE_ICON_FILE "ouasim")
    set(MACOSX_COMPOSER_ICON "${CMAKE_CURRENT_SOURCE_DIR}/res/macos/${MACOSX_BUNDLE_ICON_FILE}.icon")
    set(MACOSX_ICON_OUTPUT_DIR "$<TARGET_BUNDLE_CONTENT_DIR:${target_name}>/Resources")

    find_program(ACTOOL_EXECUTABLE actool REQUIRED)

    # POST_BUILD rather than a separate target, so building only the app target still ships Assets.car and the .icns.
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${MACOSX_ICON_OUTPUT_DIR}"
        COMMAND "${ACTOOL_EXECUTABLE}"
            "${MACOSX_COMPOSER_ICON}"
            --app-icon "${MACOSX_BUNDLE_ICON_FILE}"
            --compile "${MACOSX_ICON_OUTPUT_DIR}"
            --output-partial-info-plist "${PROJECT_BINARY_DIR}/assetcatalog_generated_info.plist"
            --minimum-deployment-target "11.0"
            --platform macosx
            --target-device mac
            --include-all-app-icons
            --enable-on-demand-resources NO
            --development-region en
        COMMENT "Compiling macOS Icon Composer app icon"
        VERBATIM
    )

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/ouasim.plist.in"
        "${PROJECT_BINARY_DIR}/Info.plist"
        @ONLY
    )

    set_target_properties(${target_name} PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_INFO_PLIST "${PROJECT_BINARY_DIR}/Info.plist"
    )
endfunction()
