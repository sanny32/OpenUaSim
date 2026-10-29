cmake_minimum_required(VERSION 3.22)

set(PROJECT_NAME ouasim)
set(PRODUCT_NAME OpenUaSim)
set(OUASIM_DEPLOY_TOOL "${CMAKE_COMMAND}")
set(OUASIM_OPENSSL_BIN "${FIXTURE_ROOT}/missing-openssl")
set(OUASIM_WITH_ENCRYPTION OFF)
configure_file("${OUASIM_SOURCE_ROOT}/src/cmake/deploy_runtime.cmake.in"
    "${FIXTURE_ROOT}/deploy.cmake" @ONLY)
file(READ "${FIXTURE_ROOT}/deploy.cmake" deploy_script)

foreach(windows IN ITEMS TRUE FALSE)
    file(WRITE "${FIXTURE_ROOT}/run.cmake"
        "set(CMAKE_HOST_WIN32 ${windows})\nset(CMAKE_INSTALL_PREFIX \"${FIXTURE_ROOT}/stage\")\n${deploy_script}")
    execute_process(COMMAND "${CMAKE_COMMAND}" -P "${FIXTURE_ROOT}/run.cmake"
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(result EQUAL 0 OR NOT error MATCHES "Qt runtime deployment failed")
        message(FATAL_ERROR "Deployment failure was not propagated: ${output}${error}")
    endif()
endforeach()

set(OUASIM_WITH_ENCRYPTION ON)
configure_file("${OUASIM_SOURCE_ROOT}/src/cmake/deploy_runtime.cmake.in"
    "${FIXTURE_ROOT}/deploy.cmake" @ONLY)
file(READ "${FIXTURE_ROOT}/deploy.cmake" deploy_script)
file(WRITE "${FIXTURE_ROOT}/run.cmake" "set(CMAKE_HOST_WIN32 TRUE)\n${deploy_script}")
execute_process(COMMAND "${CMAKE_COMMAND}" -P "${FIXTURE_ROOT}/run.cmake"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "OpenSSL runtime DLLs not found")
    message(FATAL_ERROR "Missing OpenSSL was not rejected: ${output}${error}")
endif()
