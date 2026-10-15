option(OUASIM_WITH_QUASERVER "Fetch QUaServer and link it into the server core" OFF)
set(OUASIM_QUASERVER_TAG "master" CACHE STRING "QUaServer git tag or commit to fetch")

if(OUASIM_WITH_QUASERVER)
    include(FetchContent)

    set(QUASERVER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(QUASERVER_BUILD_TESTS OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(QUaServer
        GIT_REPOSITORY "https://github.com/sanny32/QUaServer.git"
        GIT_TAG        "${OUASIM_QUASERVER_TAG}"
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(QUaServer)
endif()

function(ouasim_link_quaserver target_name)
    if(OUASIM_WITH_QUASERVER)
        target_link_libraries(${target_name} PRIVATE QUaServer::QUaServer)
        target_compile_definitions(${target_name} PRIVATE HAVE_QUASERVER)
    endif()
endfunction()
