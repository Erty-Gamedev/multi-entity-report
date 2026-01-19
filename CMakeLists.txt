cmake_minimum_required(VERSION 3.28)

set(MER_PROJECT_NAME         "mer")
set(MER_PROJECT_DESCRIPTION  "'Entity Report'-like search for an entire GoldSrc maps folder ")
set(MER_AUTHOR_DOMAIN        "https://github.com/erty-gamedev/multi-entity-report")
set(MER_AUTHOR_MAINTAINER    "erty.gamedev@gmail.com")
set(MER_VERSION_MAJOR        "1")
set(MER_VERSION_MINOR        "0")
set(MER_VERSION_PATCH        "0")
set(MER_VERSION              "${MER_VERSION_MAJOR}.${MER_VERSION_MINOR}.${MER_VERSION_PATCH}")
set(MER_NAME_VERSION         "Multi Entity Report v${MER_VERSION}")

add_compile_definitions(MER_NAME_VERSION="${MER_NAME_VERSION}")

project(${MER_PROJECT_NAME})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(PUBLIC
    "${PROJECT_SOURCE_DIR}/vendor/logging/src"
    "${PROJECT_SOURCE_DIR}/vendor/doctest/doctest"
)

add_executable(${MER_PROJECT_NAME}
    src/main.cpp
    src/mer.cpp
    src/mer.h
    src/utils.cpp
    src/utils.h
)

set_target_properties(${MER_PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "bin/debug"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "bin/debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "bin/release"
)

include_directories(tests
    "${PROJECT_SOURCE_DIR}/src"
)

add_executable(tests
    tests/main.cpp
    tests/test_query.cpp
    src/mer.cpp
    src/utils.cpp
)
