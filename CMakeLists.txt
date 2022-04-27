cmake_minimum_required(VERSION 3.9)

include(cmake/Test.cmake)
include(cmake/Deploy.cmake)

project(make_contiguous_objects CXX)

option(MCO_BUILD_TESTS "Whether to build the tests" ON)

set(MCO_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/make_contiguous_objects")

set(MCO_VERSION "0.1.0")

message(STATUS "Configuring make_contiguous_objects ${MCO_VERSION}")

add_header_library(make_contiguous_objects "${MCO_INCLUDE_DIR}/mco.hpp" "LICENSE")
set_target_properties(make_contiguous_objects PROPERTIES
    INTERFACE_LIB_VERSION ${MCO_VERSION}
    INTERFACE_COMPILE_FEATURES cxx_std_17
)

deploy_header_library(make_contiguous_objects)

if(MCO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
