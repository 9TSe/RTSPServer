cmake_minimum_required(VERSION 3.10)
project(RTSPServer)

set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread ")

file(GLOB SRC_SCHEDULER ${CMAKE_CURRENT_SOURCE_DIR}/Scheduler/*.cpp)
file(GLOB SRC_LIVE ${CMAKE_CURRENT_SOURCE_DIR}/Live/*.cpp)
set(MAIN ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

include_directories(${CMAKE_SOURCE_DIR}/Scheduler)
include_directories(${CMAKE_SOURCE_DIR}/Live)

add_executable(RTSPServer ${SRC_LIVE} ${SRC_SCHEDULER} ${MAIN})
target_link_libraries(RTSPServer pthread)
