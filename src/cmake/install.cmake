include_guard(GLOBAL)
include("${CMAKE_CURRENT_LIST_DIR}/licenses.cmake")

function(ouasim_configure_install)
    if(WIN32)
        include("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/install_windows.cmake")
    elseif(APPLE)
        include("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/install_macos.cmake")
    else()
        include("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/install_linux.cmake")
    endif()
endfunction()

function(ouasim_install_runtime)
    configure_file("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/deploy_runtime.cmake.in"
        "${PROJECT_BINARY_DIR}/deploy_runtime.cmake" @ONLY)
    install(SCRIPT "${PROJECT_BINARY_DIR}/deploy_runtime.cmake")
endfunction()
