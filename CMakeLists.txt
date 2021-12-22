cmake_minimum_required(VERSION 3.17.3)
project(bsreadrepeater)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(Pkg1_ROOT "/home/werder_d/dev/pkg1")
find_package(Pkg1 REQUIRED)
message("Pkg1 ${Pkg1_DIR}")

set(ZeroMQ_ROOT "/home/werder_d/software/zeromq-4.3.4/build")
find_package(ZeroMQ REQUIRED)
message("ZeroMQ ${ZeroMQ_DIR}")

add_subdirectory(src)
