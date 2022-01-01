cmake_minimum_required(VERSION 3.17.3)
project(bsreadrepeater)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# TODO
set(Pkg1_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../pkg1")
find_package(Pkg1 REQUIRED)
message("Pkg1 ${Pkg1_DIR}")
message("Pkg1 ${CMAKE_CURRENT_SOURCE_DIR}")

# TODO
set(ZeroMQ_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../software/zeromq-4.3.4/build")
find_package(ZeroMQ REQUIRED)
message("ZeroMQ ${ZeroMQ_DIR}")
message("ZeroMQ ${ZeroMQ}")

find_package(PkgConfig REQUIRED)
pkg_check_modules(glib2pkg REQUIRED IMPORTED_TARGET glib-2.0)

link_libraries(libzmq)
# TODO
include_directories(include)
# TODO
include_directories("${ZeroMQ_ROOT}/../include")

add_subdirectory(src)
