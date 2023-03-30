include_directories(
        ../include
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(het_pattern_match
        src/pattern_match.cpp
        )

target_link_libraries(het_pattern_match
        dl
        asan
)
