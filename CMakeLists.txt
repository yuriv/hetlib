cmake_minimum_required(VERSION 3.11)
project(hetlib VERSION 0.0.1 LANGUAGES CXX DESCRIPTION "HetLib is a header-only heterogeneous container library")

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
if (NOT CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    add_library(het::hetlib ALIAS hetlib)
endif()
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

add_subdirectory(examples)

enable_testing()

add_subdirectory(tests)

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

if (NOT HETLIB_BUILD_PACKAGE)
    return()
endif()

list(APPEND source-generators TBZ2 TGZ TXZ ZIP)

if (CMAKE_HOST_WIN32)
    list(APPEND binary-generators "WIX")
endif()

if (HETLIB_BUILD_PACKAGE_DEB)
    list(APPEND binary-generators "DEB")
endif()

if (HETLIB_BUILD_RPM)
    list(APPEND binary-generators "RPM")
endif()

set(CPACK_SOURCE_GENERATOR ${source-generators})
set(CPACK_GENERATOR ${binary-generators})

set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}")

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Yuri Volodine")

list(APPEND CPACK_SOURCE_IGNORE_FILES /.git/ /build/ .gitignore .DS_Store)

include(CPack)
