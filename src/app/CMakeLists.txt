include("${PROJECT_SOURCE_DIR}/cmake/target_macos.cmake")

add_executable(${PROJECT_NAME}
    main.cpp
    resources.qrc
)

target_link_libraries(${PROJECT_NAME} PRIVATE ouasim_ui)
ouasim_deploy_quaserver_runtime(${PROJECT_NAME})

if(WIN32)
    enable_language(RC)
    configure_file(res/windows/ouasim.rc.in "${CMAKE_CURRENT_BINARY_DIR}/ouasim.rc" @ONLY)
    set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/ouasim.rc" PROPERTIES
        OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/res/windows/ouasim.ico")
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/ouasim.rc")
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE ON)
elseif(APPLE)
    ouasim_configure_target_macos(${PROJECT_NAME})
endif()

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION .
)
