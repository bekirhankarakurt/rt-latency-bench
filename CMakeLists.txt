cmake_minimum_required(VERSION 3.16)
project(rt-latency-bench VERSION 1.0.0 LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(RTLB_BUILD_TESTS "Build the unit tests" ON)
option(RTLB_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(RTLB_WERROR "Treat compiler warnings as errors" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_library(rtlb_options INTERFACE)
target_compile_options(rtlb_options INTERFACE
  -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
if(RTLB_WERROR)
  target_compile_options(rtlb_options INTERFACE -Werror)
endif()
if(RTLB_SANITIZE)
  target_compile_options(rtlb_options INTERFACE
    -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=undefined)
  target_link_options(rtlb_options INTERFACE -fsanitize=address,undefined)
endif()

file(GLOB RTLB_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/*.cpp)
add_library(rtlb_core STATIC ${RTLB_SOURCES})
target_include_directories(rtlb_core PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_compile_definitions(rtlb_core PUBLIC RTLB_VERSION="${PROJECT_VERSION}")
target_link_libraries(rtlb_core PUBLIC rtlb_options Threads::Threads)

if(EXISTS ${PROJECT_SOURCE_DIR}/apps/main.cpp)
  add_executable(rtlb apps/main.cpp)
  target_link_libraries(rtlb PRIVATE rtlb_core)
  install(TARGETS rtlb RUNTIME DESTINATION bin)
endif()

if(RTLB_BUILD_TESTS)
  enable_testing()
  file(GLOB RTLB_TEST_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/tests/test_*.cpp)
  foreach(test_source ${RTLB_TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name} ${test_source})
    target_include_directories(${test_name} PRIVATE ${PROJECT_SOURCE_DIR}/tests)
    target_link_libraries(${test_name} PRIVATE rtlb_core)
    add_test(NAME ${test_name} COMMAND ${test_name})
  endforeach()
endif()
