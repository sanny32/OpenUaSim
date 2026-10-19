function(ouasim_add_build_script_test)
    add_executable(ouasim_tests_build_scripts test_buildscripts.cpp)
    target_link_libraries(ouasim_tests_build_scripts PRIVATE Qt6::Test)
    target_compile_definitions(ouasim_tests_build_scripts PRIVATE
        OUASIM_SOURCE_DIR="${PROJECT_SOURCE_DIR}/..")
    add_test(NAME ouasim_tests_build_scripts COMMAND ouasim_tests_build_scripts)
    ouasim_configure_test_environment(ouasim_tests_build_scripts)
endfunction()

function(ouasim_configure_test_environment test_name)
    set(environment_modifications
        "PATH=path_list_prepend:${QT_BINARY_DIR}"
        "QT_PLUGIN_PATH=path_list_prepend:${QT_BINARY_DIR}/../plugins")
    set(environment "QT_QPA_PLATFORM=offscreen")
    if(WIN32)
        # The offscreen platform ships no fonts on Windows; without them text has no size.
        file(TO_CMAKE_PATH "$ENV{WINDIR}/Fonts" font_dir)
        list(APPEND environment "QT_QPA_FONTDIR=${font_dir}")
    endif()
    set_tests_properties(${test_name} PROPERTIES
        ENVIRONMENT "${environment}"
        ENVIRONMENT_MODIFICATION "${environment_modifications}"
        LABELS "ouasim")
endfunction()

# Redirects QSettings into a per-process temporary directory; every test links it.
function(ouasim_configure_shared_test_objects)
    add_library(ouasim_test_settings_isolation OBJECT
        "${CMAKE_CURRENT_SOURCE_DIR}/testsettingsisolation.cpp"
    )
    target_link_libraries(ouasim_test_settings_isolation PRIVATE Qt${QT_VERSION_MAJOR}::Core)

    add_library(ouasim_test_mock_data OBJECT
        "${CMAKE_CURRENT_SOURCE_DIR}/mock/mockdata.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/mock/mockdata.h"
    )
    target_link_libraries(ouasim_test_mock_data PRIVATE ouasim_core)
endfunction()

function(ouasim_register_test test_name source_file)
    add_executable(${test_name} "${CMAKE_CURRENT_SOURCE_DIR}/${source_file}")
    target_sources(${test_name} PRIVATE
        $<TARGET_OBJECTS:ouasim_test_settings_isolation>
        $<TARGET_OBJECTS:ouasim_test_mock_data>
    )
    target_include_directories(${test_name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(${test_name} PRIVATE
        Qt${QT_VERSION_MAJOR}::Test
        Qt${QT_VERSION_MAJOR}::Network
        Qt${QT_VERSION_MAJOR}::Widgets
    )
    target_compile_definitions(${test_name} PRIVATE OUASIM_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
    ouasim_deploy_quaserver_runtime(${test_name})
    add_test(NAME ${test_name} COMMAND ${test_name})
    ouasim_configure_test_environment(${test_name})
endfunction()

function(ouasim_add_test test_name source_file)
    ouasim_register_test(${test_name} ${source_file})
    target_link_libraries(${test_name} PRIVATE ouasim_core)
endfunction()

function(ouasim_add_server_test test_name source_file)
    ouasim_add_test(${test_name} ${source_file})
    target_sources(${test_name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/uatestclient.cpp")
endfunction()

function(ouasim_add_ui_test test_name source_file)
    ouasim_register_test(${test_name} ${source_file})
    target_sources(${test_name} PRIVATE "${PROJECT_SOURCE_DIR}/app/resources.qrc")
    target_link_libraries(${test_name} PRIVATE ouasim_ui)
endfunction()
