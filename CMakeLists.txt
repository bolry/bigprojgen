cmake_minimum_required(VERSION 2.8)
project(bigprojgen CXX)

if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif(CMAKE_COMPILER_IS_GNUCXX)

add_executable(bigprojgen bigprojgen2.cpp)
