cmake_minimum_required(VERSION 3.11)

include(cmake/Modules/prelude.cmake)

project(hetlib VERSION 0.0.1 LANGUAGES CXX DESCRIPTION "HetLib is a header-only heterogeneous container library")

include(cmake/Modules/project-is-top-level.cmake)
include(cmake/Modules/variables.cmake)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HETLIB_BUILD_PACKAGE "Build package files as well" OFF)

find_package(Git REQUIRED)

include(FetchContent)

FetchContent_Declare(expected GIT_REPOSITORY https://github.com/TartanLlama/expected GIT_TAG master)
FetchContent_MakeAvailable(expected)
set(EXPECTED_INCLUDE_DIR ${expected_SOURCE_DIR}/include)
message("expected:\n${EXPECTED_INCLUDE_DIR}")

#include(ExternalProject)
#
#ExternalProject_Add(expected
#        PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/3rd/expected
#        GIT_REPOSITORY https://github.com/TartanLlama/expected
#        TIMEOUT 10
#        UPDATE_COMMAND ${GIT_EXECUTABLE} pull
#        #CONFIGURE_COMMAND ""
#        #BUILD_COMMAND ""
#        #INSTALL_COMMAND ""
#        LOG_DOWNLOAD ON
#        )
#ExternalProject_Get_Property(expected source_dir)
#set(EXPECTED_INCLUDE_DIR ${source_dir}/include)
#message("expected:\n" ${EXPECTED_INCLUDE_DIR})

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
    include/
    )
set(INTERFACE_FILES
    include/het/
    )

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options("-Wall" "-Wextra" "-Wconversion" "-Wpedantic" "-fsanitize=address")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options("-Wall" "-Wextra" "-Wconversion" "-Wpedantic" "-fconcepts" "-fsanitize=address")
endif()

configure_file(version_config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/generated/version_config.h)

add_library(hetlib INTERFACE)
add_library(het::hetlib ALIAS hetlib)

target_sources(hetlib INTERFACE
    FILE_SET HEADERS 
    BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES ${INTERFACE_FILES}
    )
target_compile_features(hetlib INTERFACE cxx_std_20)

include_directories(
        ${EXPECTED_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

if (${PROJECT_NAME}_BUILD_PACKAGE)
    set(${PROJECT_NAME}_PACKAGE_FILES ${INTERFACE_FILES} CMakeLists.txt BUILDING.md LICENSE README.md version_config.h.in)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_src.tar.gz
            COMMAND ${CMAKE_COMMAND} -E tar c ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_src.tar.gz --format=gnutar -- ${${PROJECT_NAME}_PACKAGE_FILES}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            DEPENDS ${${PROJECT_NAME}_PACKAGE_FILES})
    add_custom_target(${PROJECT_NAME}_PACKAGE DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_src.tar.gz)
endif()

configure_package_config_file(
        "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}-config.cmake.in"
        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
        INSTALL_DESTINATION "share/cmake/${PROJECT_NAME}")

write_basic_package_version_file(
        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake"
        COMPATIBILITY SameMajorVersion
        ARCH_INDEPENDENT)

target_include_directories(hetlib
        INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

install(TARGETS hetlib FILE_SET HEADERS)

#install(TARGETS hetlib
#        EXPORT ${PROJECT_NAME}-targets
#)
#
#install(EXPORT ${PROJECT_NAME}-targets
#        DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${PROJECT_NAME}"
#        NAMESPACE het::
#        FILE "${PROJECT_NAME}-targets.cmake")
#
#install(FILES
#        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake"
#        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
#        DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${PROJECT_NAME}")
#
#install(DIRECTORY "include/" TYPE INCLUDE)

if(NOT ${PROJECT_NAME}_DEVELOPER_MODE)
    return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
    message(AUTHOR_WARNING "Developer mode is intended for developers of ${PROJECT_NAME}")
endif()

if (BUILD_TESTING)
    add_subdirectory(examples)

    enable_testing()

    add_subdirectory(tests)
endif()

if (NOT ${PROJECT_NAME}_BUILD_PACKAGE)
    return()
endif()

list(APPEND source-generators TBZ2 TGZ TXZ ZIP)

if (CMAKE_HOST_WIN32)
    list(APPEND binary-generators "WIX")
endif()

if (${PROJECT_NAME}_BUILD_PACKAGE_DEB)
    list(APPEND binary-generators "DEB")
endif()

if (${PROJECT_NAME}_BUILD_RPM)
    list(APPEND binary-generators "RPM")
endif()

set(CPACK_SOURCE_GENERATOR ${source-generators})
set(CPACK_GENERATOR ${binary-generators})

set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}")

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Yuri Volodine")

list(APPEND CPACK_SOURCE_IGNORE_FILES /.git/ /build/ /cmake-build-*/ .gitignore .DS_Store)

include(CPack)
