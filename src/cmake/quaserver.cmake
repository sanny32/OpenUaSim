set(OUASIM_QUASERVER_TAG "master" CACHE STRING "QUaServer git tag or commit to fetch")
option(OUASIM_WITH_ENCRYPTION "Build QUaServer with OpenSSL encryption support" ON)

include(FetchContent)

set(QUASERVER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(QUASERVER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(QUASERVER_ENCRYPTION ${OUASIM_WITH_ENCRYPTION} CACHE BOOL "" FORCE)
# Limit and off-normal alarms on variables; QUaServer turns on events and the full namespace zero with them.
set(QUASERVER_ALARMS_CONDITIONS ON CACHE BOOL "" FORCE)
set(QUASERVER_HISTORIZING ON CACHE BOOL "" FORCE)

FetchContent_Declare(QUaServer
    GIT_REPOSITORY "https://github.com/sanny32/QUaServer.git"
    GIT_TAG        "${OUASIM_QUASERVER_TAG}"
)
FetchContent_MakeAvailable(QUaServer)

function(ouasim_link_quaserver target_name)
    target_link_libraries(${target_name} PUBLIC QUaServer::QUaServer)
    if(OUASIM_WITH_ENCRYPTION)
        target_compile_definitions(${target_name} PUBLIC OUASIM_HAVE_ENCRYPTION)
    endif()
endfunction()

function(ouasim_deploy_quaserver_runtime target_name)
    if(OUASIM_WITH_ENCRYPTION AND WIN32)
        quaserver_deploy_openssl_runtime(${target_name})
    endif()
endfunction()
