cmake_minimum_required(VERSION 3.10)
project(9TSe_Rtsp_Server)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread ")

file(GLOB SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB HEAD ${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${HEAD})
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR})
add_executable(app ${SRC})



#cmake_minimum_required(VERSION 3.10)
#project(9TSe_Rtsp_Server)
#
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread ")
#
#set(CMAKE_BUILD_TYPE DEBUG)
#set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb") #无优化 , 启用警告 , 生成调试信息(-g2 -ggdb)
#set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")
#
#set(INCLUDE_DIR /usr/local/include)
#set(LIB_DIR /usr/local/lib)
#include_directories(${INCLUDE_DIR})
#link_directories(${LIB_DIR})
#
#add_executable(
#	9TSe_Rtsp_Server
#	Buffer.cpp
#	InetAddress.cpp
#	MediaSessionManager.cpp
#	MediaSession.cpp
#	AACFileMediaSource.cpp
#	H264FileMediaSource.cpp
#	Rtp.cpp
#	MediaSource.cpp
#	RtpMediaSource.cpp
#	AACSink.cpp
#	H264Sink.cpp
#	H264FileSink.cpp
#	AACFileSink.cpp
#	Sink.cpp
#	RtspConnection.cpp
#	RtspServer.cpp
#	TcpConnection.cpp
#	Event.cpp
#	EventScheduler.cpp
#	Poller.cpp
#	SelectPoller.cpp
#	SocketsOps.cpp
#	Thread.cpp
#	ThreadPool.cpp
#	Timer.cpp
#	UsageEnvironment.cpp
#	main.cpp
#)