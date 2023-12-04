cmake_minimum_required(VERSION 3.6)
project(hetlib VERSION 0.0.1 LANGUAGES CXX DESCRIPTION "HetLib is a header-only heterogeneous container library")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Git REQUIRED)

include(FetchContent)

FetchContent_Declare(doctest GIT_REPOSITORY https://github.com/doctest/doctest.git GIT_TAG master)
FetchContent_MakeAvailable(doctest)
set(DOCTEST_INCLUDE_DIR ${doctest_SOURCE_DIR}/doctest)
message("doctest:\n" ${DOCTEST_INCLUDE_DIR})

include(ExternalProject)

ExternalProject_Add(expected
        PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/3rd/expected
        GIT_REPOSITORY https://github.com/TartanLlama/expected
        TIMEOUT 10
        UPDATE_COMMAND ${GIT_EXECUTABLE} pull
        #CONFIGURE_COMMAND ""
        #BUILD_COMMAND ""
        INSTALL_COMMAND ""
        LOG_DOWNLOAD ON
        )
ExternalProject_Get_Property(expected source_dir)
set(EXPECTED_INCLUDE_DIR ${source_dir}/include)
message("expected:\n" ${EXPECTED_INCLUDE_DIR})

ExternalProject_Add(
    benchmark
    PREFIX ${CMAKE_BINARY_DIR}/3rd/benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    CMAKE_ARGS "-DCMAKE_BUILD_TYPE=Release" "-DCMAKE_INSTALL_LIBDIR=lib64" "-DBENCHMARK_DOWNLOAD_DEPENDENCIES=ON" "-DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/3rd/benchmark"
    GIT_TAG main
    TIMEOUT 10
    UPDATE_COMMAND ${GIT_EXECUTABLE} pull
    LOG_DOWNLOAD ON
)

# Expose required variable (GBENCH_INCLUDE_DIR) to parent scope
ExternalProject_Get_Property(benchmark install_dir)
set(GBENCH_INCLUDE_DIR ${install_dir}/include CACHE INTERNAL "")

ExternalProject_Get_Property(benchmark binary_dir)
set(GBENCH_LIB_DIR ${install_dir}/lib64 CACHE INTERNAL "")

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
    include/het/details/domains.h
    include/het/details/error.h
    include/het/details/expected.h
    include/het/details/match.h
    include/het/details/type_ctors_util.h
    include/het/details/typesafe.h
    include/het/details/uncommon_definitions.h
    include/het/het.h
    include/het/het_container.h
    include/het/het_keyvalue.h
    include/het/het_value.h
    )
set(INTERFACE_FILES
    include/het/het.h
    include/het/het_container.h
    include/het/het_keyvalue.h
    include/het/het_value.h
    )

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options("-Wall" "-Wextra" "-Wconversion" "-Wpedantic" "-fsanitize=address")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options("-Wall" "-Wextra" "-Wconversion" "-Wpedantic" "-fconcepts" "-fsanitize=address")
endif()

configure_file(version_config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/generated/version_config.h)

add_library(hetlib INTERFACE)
target_sources(hetlib INTERFACE 
    FILE_SET HEADERS 
    BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${INTERFACE_FILES}
    )
target_compile_features(hetlib INTERFACE cxx_std_20)

include_directories(
        ${EXPECTED_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include/generated
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

add_subdirectory(examples)

enable_testing()

add_subdirectory(tests)

install(TARGETS hetlib FILE_SET HEADERS)
