function(ouasim_add_build_script_test)
    add_executable(ouasim_tests_build_scripts test_buildscripts.cpp)
    target_link_libraries(ouasim_tests_build_scripts PRIVATE Qt6::Test)
    target_compile_definitions(ouasim_tests_build_scripts PRIVATE
        OUASIM_SOURCE_DIR="${PROJECT_SOURCE_DIR}/..")
    add_test(NAME ouasim_tests_build_scripts COMMAND ouasim_tests_build_scripts)
    ouasim_configure_test_environment(ouasim_tests_build_scripts)
endfunction()

function(ouasim_add_app_metadata_test)
    add_executable(ouasim_tests_app_metadata test_appmetadata.cpp)
    target_link_libraries(ouasim_tests_app_metadata PRIVATE Qt6::Test)
    if(WIN32)
        add_dependencies(ouasim_tests_app_metadata ${PROJECT_NAME})
        target_link_libraries(ouasim_tests_app_metadata PRIVATE version)
        target_compile_definitions(ouasim_tests_app_metadata PRIVATE
            OUASIM_EXECUTABLE_PATH="$<TARGET_FILE:${PROJECT_NAME}>")
    endif()
    add_test(NAME ouasim_tests_app_metadata COMMAND ouasim_tests_app_metadata)
    ouasim_configure_test_environment(ouasim_tests_app_metadata)
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
    ouasim_configure_test_compilation(ouasim_test_mock_data ouasim_core)
    # Header parsing dominates test compile time; every test links these objects, so they own the one shared PCH.
    target_precompile_headers(ouasim_test_mock_data PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/testpch.h")

    add_library(ouasim_test_ua_client OBJECT
        "${CMAKE_CURRENT_SOURCE_DIR}/uatestclient.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/uatestclient.h"
    )
    ouasim_configure_test_compilation(ouasim_test_ua_client ouasim_core)
    target_precompile_headers(ouasim_test_ua_client REUSE_FROM ouasim_test_mock_data)
endfunction()

# GCC ignores a reused PCH unless the macros match exactly, so every test source shares these settings.
function(ouasim_configure_test_compilation target_name project_library)
    target_include_directories(${target_name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(${target_name} PRIVATE
        Qt${QT_VERSION_MAJOR}::Test
        Qt${QT_VERSION_MAJOR}::Network
        Qt${QT_VERSION_MAJOR}::Widgets
        ${project_library}
    )
    target_compile_definitions(${target_name} PRIVATE
        OUASIM_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
        OUASIM_EXAMPLE_NODESETS_DIR="${PROJECT_SOURCE_DIR}/../examples/nodesets")
endfunction()

# Each test links exactly one project library; ouasim_ui already brings ouasim_core, and a second copy upsets Apple ld.
function(ouasim_add_test_executable test_name source_file project_library)
    add_executable(${test_name} "${CMAKE_CURRENT_SOURCE_DIR}/${source_file}")
    target_sources(${test_name} PRIVATE
        $<TARGET_OBJECTS:ouasim_test_settings_isolation>
        $<TARGET_OBJECTS:ouasim_test_mock_data>
    )
    ouasim_configure_test_compilation(${test_name} ${project_library})
    target_precompile_headers(${test_name} REUSE_FROM ouasim_test_mock_data)
    ouasim_deploy_quaserver_runtime(${test_name})
    add_test(NAME ${test_name} COMMAND ${test_name})
    ouasim_configure_test_environment(${test_name})
endfunction()

function(ouasim_add_test test_name source_file)
    ouasim_add_test_executable(${test_name} ${source_file} ouasim_core)
endfunction()

function(ouasim_add_server_test test_name source_file)
    ouasim_add_test(${test_name} ${source_file})
    ouasim_link_test_ua_client(${test_name})
endfunction()

function(ouasim_link_test_ua_client test_name)
    target_sources(${test_name} PRIVATE $<TARGET_OBJECTS:ouasim_test_ua_client>)
endfunction()

function(ouasim_add_ui_test test_name source_file)
    ouasim_add_test_executable(${test_name} ${source_file} ouasim_ui)
    target_sources(${test_name} PRIVATE "${PROJECT_SOURCE_DIR}/app/resources.qrc")
endfunction()

function(ouasim_add_app_icon_test)
    ouasim_add_ui_test(ouasim_tests_app_icon test_appicon.cpp)
    target_compile_definitions(ouasim_tests_app_icon PRIVATE
        OUASIM_MAC_ICON_PATH="${PROJECT_SOURCE_DIR}/app/res/macos/ouasim.icon/Assets/ouasim-mac.png")
    if(WIN32)
        add_dependencies(ouasim_tests_app_icon ${PROJECT_NAME})
        target_compile_definitions(ouasim_tests_app_icon PRIVATE
            OUASIM_EXECUTABLE_PATH="$<TARGET_FILE:${PROJECT_NAME}>")
    endif()
endfunction()
