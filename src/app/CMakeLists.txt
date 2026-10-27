include("${PROJECT_SOURCE_DIR}/cmake/target_macos.cmake")

add_executable(${PROJECT_NAME}
    main.cpp
    resources.qrc
)

target_link_libraries(${PROJECT_NAME} PRIVATE ouasim_ui)
ouasim_deploy_quaserver_runtime(${PROJECT_NAME})

if(WIN32)
    enable_language(RC)
    set_source_files_properties(res/windows/ouasim.rc PROPERTIES
        OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/res/windows/ouasim.ico")
    target_sources(${PROJECT_NAME} PRIVATE res/windows/ouasim.rc)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE ON)
elseif(APPLE)
    ouasim_configure_target_macos(${PROJECT_NAME})
endif()

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION .
)
