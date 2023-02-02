cmake_minimum_required(VERSION 3.6)
project(hetlib VERSION 0.0.1 LANGUAGES CXX DESCRIPTION "HetLib is a compile time heterogeneous container library")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

include(ExternalProject)
find_package(Git REQUIRED)

ExternalProject_Add(
        doctest
        PREFIX ${CMAKE_BINARY_DIR}/3rd/doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        TIMEOUT 10
        UPDATE_COMMAND ${GIT_EXECUTABLE} pull
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        LOG_DOWNLOAD ON
)

# Expose required variable (DOCTEST_INCLUDE_DIR) to parent scope
ExternalProject_Get_Property(doctest source_dir)
set(DOCTEST_INCLUDE_DIR ${source_dir}/doctest CACHE INTERNAL "Path to include folder for doctest")

add_definitions(-DHETLIB_AUTHOR="Yuri Volodine")
add_definitions(-DHETLIB_PRODUCT="hetlib")
add_definitions(-DHETLIB_SUITE="Reg")
add_definitions(-DHETLIB_VERSION="${CMAKE_PROJECT_VERSION}")
add_definitions(-DHETLIB_DESCRIPTION="${CMAKE_PROJECT_DESCRIPTION}")
add_definitions(-DHETLIB_FULLNAME="hetlib[Reg]")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fPIC -O0 -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fPIC -Wall -Wextra -Wpedantic -O2 -DNDEBUG")
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
set(SOURCE_FILES
    include/uncommon_definitions.h
    include/het/het.h)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options("-Wall" "-Wextra" "-Wconversion" "-Wpedantic")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options("-Wall" "-Wextra" "-Wconversion" "-Wpedantic" "-fconcepts")
endif()

configure_file(version_config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/generated/version_config.h)

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/include/generated
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

add_subdirectory(tests)
