cmake_minimum_required(VERSION 3.17.3)
project(bsreadrepeater LANGUAGES C CXX)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -march=sandybridge")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=sandybridge")

set(ZeroMQ_ROOT "${ZMQ_BUILD_DIR}")
find_package(ZeroMQ REQUIRED)
message("ZeroMQ ${ZeroMQ_DIR}")
message("ZeroMQ ${ZeroMQ}")

find_package(PkgConfig REQUIRED)
pkg_check_modules(glib2pkg REQUIRED IMPORTED_TARGET glib-2.0)

add_subdirectory(src)
