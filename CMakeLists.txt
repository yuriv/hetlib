cmake_minimum_required(VERSION 3.6)
project(hetlib VERSION 0.0.1 LANGUAGES CXX DESCRIPTION "HetLib is a compile time heterogeneous container library")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_definitions(-DHETLIB_ORGANIZATION="Oversun, Ltd.")
add_definitions(-DHETLIB_PRODUCT="Het library")
add_definitions(-DHETLIB_SUITE="Dev")
add_definitions(-DHETLIB_VERSION="${CMAKE_PROJECT_VERSION}")
add_definitions(-DHETLIB_DESCRIPTION="${CMAKE_PROJECT_DESCRIPTION}")
add_definitions(-DHETLIB_FULLNAME="Hetlib[DevSuite]")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fPIC -O0 -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_INSTRUMENTEDRELEASE "${CMAKE_CXX_FLAGS_INSTRUMENTEDRELEASE} -fPIC -Wall -Wextra -Wpedantic -g -O2")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fPIC -Wall -Wextra -Wpedantic -O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_PGORELEASE "${CMAKE_CXX_FLAGS_PGORELEASE} -fPIC -Wall -Wextra -Wpedantic -O3 -DNDEBUG")
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;InstrumentedRelease;PGORelease" CACHE STRING "" FORCE)
set(SOURCE_FILES
    include/uncommon_definitions.h
    include/het/het.h)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options("-Wall" "-Wextra" "-Wconversion" "-Wpedantic")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options("-fconcepts" "-Wall" "-Wextra" "-Wconversion" "-Wpedantic" "-fconcepts"
        #"-fsanitize=address"
            )
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE})

configure_file(version_config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/generated/version_config.h)

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/include/generated
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

add_subdirectory(tests)
