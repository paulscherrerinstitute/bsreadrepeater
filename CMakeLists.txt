cmake_minimum_required(VERSION 3.17.3)
project(bsreadrepeater LANGUAGES C CXX)

file(READ "version.txt" TMP)
string(REGEX MATCH "([-.0-9]*)" RXM ${TMP})
set(BSREP_VERSION ${RXM})
message("BSREP_VERSION [${BSREP_VERSION}]")

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -march=sandybridge")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=sandybridge")

if(DEFINED ZMQ_BUILD_DIR)
set(ZeroMQ_ROOT "${ZMQ_BUILD_DIR}")
else()
set(ZeroMQ_ROOT "/home/dominik/software/zeromq-4.3.4/install")
endif()

message("ZeroMQ_ROOT ${ZeroMQ_ROOT}")

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_path(ZMQ_INCLUDE_DIR zmq.h "${ZeroMQ_ROOT}/include" NO_DEFAULT_PATH)
find_library(ZMQ_LIB libzmq.so "${ZeroMQ_ROOT}/lib64" NO_DEFAULT_PATH)
message("ZMQ_INCLUDE_DIR ${ZMQ_INCLUDE_DIR}")
message("ZMQ_LIB ${ZMQ_LIB}")

#find_package(ZeroMQ REQUIRED)
#message("ZeroMQ ${ZeroMQ_DIR}")
#message("ZeroMQ ${ZeroMQ}")

find_package(PkgConfig REQUIRED)
pkg_check_modules(glib2pkg REQUIRED IMPORTED_TARGET glib-2.0)

set(CMAKE_BUILD_RPATH "$ORIGIN")

add_subdirectory(src)
