include(ExternalProject)

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

ExternalProject_Get_Property(benchmark install_dir)
set(GBENCH_INCLUDE_DIR ${install_dir}/include CACHE INTERNAL "")

ExternalProject_Get_Property(benchmark binary_dir)
set(GBENCH_LIB_DIR ${install_dir}/lib64 CACHE INTERNAL "")

include(FetchContent)

FetchContent_Declare(doctest GIT_REPOSITORY https://github.com/doctest/doctest.git GIT_TAG master)
FetchContent_MakeAvailable(doctest)
set(DOCTEST_INCLUDE_DIR ${doctest_SOURCE_DIR}/doctest)
message("doctest:\n" ${DOCTEST_INCLUDE_DIR})

message("compiler :" ${CMAKE_CXX_COMPILER_VERSION})
include_directories(
        ../include
        ${DOCTEST_INCLUDE_DIR}
        ${GBENCH_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(het_tests
        src/hetero_value.cpp
        src/hetero_keyvalue.cpp
        src/hetero_container.cpp
        )
target_compile_features(het_tests PUBLIC cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options("-fsanitize=address")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options("-fsanitize=address")
endif()

target_link_libraries(het_tests
        asan
        dl
)

add_test(test het_tests)

add_executable(het_bench
        src/hetero_value_bench.cpp
        src/hetero_keyvalue_bench.cpp
        src/hetero_container_bench.cpp
        )
target_compile_features(het_bench PUBLIC cxx_std_20)

target_link_libraries(het_bench
        asan
        dl
        pthread
        ${GBENCH_LIB_DIR}/libbenchmark.a
        ${GBENCH_LIB_DIR}/libbenchmark_main.a
        )
