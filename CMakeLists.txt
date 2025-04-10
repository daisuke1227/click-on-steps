cmake_minimum_required(VERSION 3.21)
project(cbf-lite VERSION 1.0.0)

# Set C++ standard settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# Check if building for iOS
if("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS")
    set(GLOBED_IOS ON)
endif()

# Set architecture based on platform type
if(GLOBED_IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()

# Set up source files list
set(MOD_SOURCES
    src/main.cpp
    src/async.cpp
    src/input.cpp
)

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
    list(APPEND MOD_SOURCES src/macos.mm)
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS")
    list(APPEND MOD_SOURCES src/ios.mm)
    set_source_files_properties(src/macos.mm PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Android")
    list(APPEND MOD_SOURCES src/android.cpp)
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    list(APPEND MOD_SOURCES src/windows.cpp)
else()
    message(FATAL_ERROR "Unsupported mod platform :(")
endif()

# Create the shared library for the project
add_library(${PROJECT_NAME} SHARED ${MOD_SOURCES})

# Check for Geode SDK environment variable and add it as a subdirectory
if(NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define the GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# Configure the Geode mod
setup_geode_mod(${PROJECT_NAME})