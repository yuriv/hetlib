cmake_minimum_required(VERSION 3.6)
project(hetlib VERSION 0.0.1 LANGUAGES CXX DESCRIPTION "HetLib is a header-only heterogeneous container library")

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
set(DOCTEST_INCLUDE_DIR ${source_dir}/doctest CACHE INTERNAL "")

#ExternalProject_Add(
#        googletest
#        PREFIX ${CMAKE_BINARY_DIR}/3rd/googletest
#        GIT_REPOSITORY https://github.com/google/googletest.git
#        GIT_TAG main
#        TIMEOUT 10
#        UPDATE_COMMAND ${GIT_EXECUTABLE} pull
#        CONFIGURE_COMMAND ""
#        BUILD_COMMAND ""
#        INSTALL_COMMAND ""
#        LOG_DOWNLOAD ON
#)
#
## Expose required variable (GTEST_INCLUDE_DIR) to parent scope
#ExternalProject_Get_Property(googletest source_dir)
#set(GTEST_INCLUDE_DIR ${source_dir}/googletest/googletest/include CACHE INTERNAL "")

ExternalProject_Add(
    benchmark
    PREFIX ${CMAKE_BINARY_DIR}/3rd/benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    CMAKE_ARGS "-DCMAKE_BUILD_TYPE=Release" "-DBENCHMARK_DOWNLOAD_DEPENDENCIES=ON" "-DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/3rd/benchmark"
    GIT_TAG main
    TIMEOUT 10
    UPDATE_COMMAND ${GIT_EXECUTABLE} pull
    #COMMAND "${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/3rd/benchmark/src/benchmark ${CMAKE_COMMAND} -E make_directory build"
    #CONFIGURE_COMMAND #"cmake -E chdir \"${CMAKE_BINARY_DIR}/3rd/benchmark/src/benchmark\" cmake -E make_directory \"build\""
    #    "${CMAKE_COMMAND} -E chdir \"build\" ${CMAKE_COMMAND} -DBENCHMARK_DOWNLOAD_DEPENDENCIES=on -DCMAKE_BUILD_TYPE=Release ../"
    #BUILD_COMMAND "${CMAKE_COMMAND} --build \"build\" --config Release"
    #INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
)

# Expose required variable (GBENCH_INCLUDE_DIR) to parent scope
ExternalProject_Get_Property(benchmark install_dir)
set(GBENCH_INCLUDE_DIR ${install_dir}/include CACHE INTERNAL "")

ExternalProject_Get_Property(benchmark binary_dir)
set(GBENCH_LIB_DIR ${install_dir}/lib64 CACHE INTERNAL "")

#add_subdirectory(
#        ${source_dir}
#        ${binary_dir})

add_definitions(-DHETLIB_AUTHOR="Yuri Volodine")
add_definitions(-DHETLIB_PRODUCT="hetlib")
add_definitions(-DHETLIB_SUITE="Std")
add_definitions(-DHETLIB_VERSION="${CMAKE_PROJECT_VERSION}")
add_definitions(-DHETLIB_DESCRIPTION="${CMAKE_PROJECT_DESCRIPTION}")
add_definitions(-DHETLIB_FULLNAME="hetlib[Std]")

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

enable_testing()

add_subdirectory(tests)
