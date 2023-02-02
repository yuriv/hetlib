include_directories(
        ../include
        ${DOCTEST_INCLUDE_DIR}
        ../3rd/benchmark/include
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
