cmake_minimum_required(VERSION 3.22)

set(arguments)
set(after_script FALSE)
math(EXPR last_argument "${CMAKE_ARGC} - 1")
foreach(index RANGE 0 ${last_argument})
    if(after_script)
        list(APPEND arguments "${CMAKE_ARGV${index}}")
    elseif(CMAKE_ARGV${index} STREQUAL CMAKE_CURRENT_LIST_FILE)
        set(after_script TRUE)
    endif()
endforeach()

set(app "${TEST_STAGE}/OpenUaSim.app")
set(unwanted_components
    PlugIns/imageformats/libqpdf.dylib
    PlugIns/platforminputcontexts/libqtvirtualkeyboardplugin.dylib
    Frameworks/QtPdf.framework
    Frameworks/QtVirtualKeyboard.framework
    Frameworks/QtVirtualKeyboardQml.framework)

if(TEST_WINDOWS)
    if(TEST_CONFIG STREQUAL Debug)
        set(mode --debug)
    else()
        set(mode --release)
    endif()
    set(expected_arguments ${mode} --compiler-runtime --force-openssl
        --plugindir "${TEST_STAGE}/plugins"
        --no-opengl-sw --no-system-d3d-compiler --no-system-dxc-compiler
        --skip-plugin-types generic,networkinformation,qmltooling
        --exclude-plugins qpdf,qsqlibase,qsqlmimer,qsqloci,qsqlodbc,qsqlpsql
        --dir "${TEST_STAGE}" "${TEST_STAGE}/ouasim.exe")
    set(phase deploy)
else()
    list(GET arguments 0 first_argument)
    if(first_argument STREQUAL app)
        set(expected_arguments "${app}" -libpath= -always-overwrite -codesign=-)
        set(phase deploy)
        foreach(component IN LISTS unwanted_components)
            if(component MATCHES "\\.framework$")
                file(MAKE_DIRECTORY "${app}/Contents/${component}")
                file(WRITE "${app}/Contents/${component}/unused" "fixture")
            else()
                file(WRITE "${app}/Contents/${component}" "fixture")
            endif()
        endforeach()
        file(WRITE "${app}/Contents/Frameworks/QtSvg.framework/required" "fixture")
    elseif(first_argument STREQUAL --force)
        set(expected_arguments --force --deep --sign - --timestamp=none "${app}")
        set(phase sign)
        foreach(component IN LISTS unwanted_components)
            if(EXISTS "${app}/Contents/${component}")
                message(FATAL_ERROR "Unused component remains before signing: ${component}")
            endif()
        endforeach()
        if(NOT EXISTS "${app}/Contents/Frameworks/QtSvg.framework/required")
            message(FATAL_ERROR "Required framework was removed")
        endif()
    else()
        set(expected_arguments --verify --deep --strict "${app}")
        set(phase verify)
    endif()
endif()

if(NOT arguments STREQUAL expected_arguments)
    message(FATAL_ERROR "Unexpected ${phase} arguments: ${arguments}")
endif()
file(APPEND "${TEST_STAGE}/calls.txt" "${phase}\n")
if(TEST_FAILURE STREQUAL phase)
    message(FATAL_ERROR "Simulated ${phase} failure")
endif()
