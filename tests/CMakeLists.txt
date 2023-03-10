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

target_link_libraries(het_tests
        dl
)

add_test(test het_tests)

add_executable(het_bench
        src/hetero_value_bench.cpp
        src/hetero_keyvalue_bench.cpp
        src/hetero_container_bench.cpp
        )

target_link_libraries(het_bench
        dl
        pthread
        ${GBENCH_LIB_DIR}/libbenchmark.a
        ${GBENCH_LIB_DIR}/libbenchmark_main.a
        )
