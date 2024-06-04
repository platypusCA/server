cmake_minimum_required(VERSION 3.15)
project(server_project VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Specify vcpkg toolchain file
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
endif()

if(NOT EXISTS ${CMAKE_TOOLCHAIN_FILE})
    message(FATAL_ERROR "Vcpkg toolchain file not found: ${CMAKE_TOOLCHAIN_FILE}")
endif()

# Enable CMake to find vcpkg packages
include("${CMAKE_TOOLCHAIN_FILE}")

# Find packages
find_package(Boost REQUIRED COMPONENTS program_options asio system filesystem)
find_package(OpenSSL REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})

add_subdirectory(src)
