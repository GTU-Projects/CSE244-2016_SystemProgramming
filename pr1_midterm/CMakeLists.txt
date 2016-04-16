cmake_minimum_required(VERSION 3.4)
project(pr1_midterm)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c90")

set(SOURCE_FILES integralGen.c)
set(SOURCE_FILES client.c)
add_executable(pr1_midterm ${SOURCE_FILES})