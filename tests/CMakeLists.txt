include_directories(
        ../include
        ${DOCTEST_INCLUDE_DIR}
        ${GBENCH_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(het_tests
        src/hetero_container.cpp
        src/hetero_value.cpp
        #src/hetero_copy_test.cpp
        )

target_link_libraries(het_tests
        dl
)

add_test(test het_tests)