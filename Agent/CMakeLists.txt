CMAKE_MINIMUM_REQUIRED(VERSION 2.8.0 FATAL_ERROR)
PROJECT(Agent)

set(SRC agent.cpp)

add_executable(Agent ${SRC})