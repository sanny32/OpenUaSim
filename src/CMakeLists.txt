cmake_minimum_required(VERSION 3.22)

project(ouasim
        VERSION 1.0.0
        DESCRIPTION "An OPC UA simulation server"
        LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PRODUCT_NAME "OpenUaSim")
set(ORGANIZATION_NAME "Ananev Alexandr")

option(OUASIM_BUILD_TESTS "Build OpenUaSim tests" OFF)

include("${PROJECT_SOURCE_DIR}/cmake/qt.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/quaserver.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/project_version.cmake")

if(OUASIM_BUILD_TESTS)
    enable_testing()
endif()

add_compile_definitions(
    APP_NAME="${PRODUCT_NAME}"
    APP_ORGANIZATION_NAME="${ORGANIZATION_NAME}"
    APP_DESCRIPTION="${PROJECT_DESCRIPTION}"
    APP_VERSION="${PROJECT_VERSION}"
    BUILD_YEAR="${CURRENT_YEAR}"
)

if(MSVC)
    add_compile_options(/utf-8)
endif()

add_subdirectory(core)
add_subdirectory(ui)
add_subdirectory(app)
if(OUASIM_BUILD_TESTS)
    add_subdirectory(tests)
endif()
