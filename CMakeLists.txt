cmake_minimum_required(VERSION 3.5)
project(Teste_de_Software)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp VRTMEM.cpp)
add_executable(Teste_de_Software ${SOURCE_FILES})