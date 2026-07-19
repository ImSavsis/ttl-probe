cmake_minimum_required(VERSION 3.16)
project(ttlprobe CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ttlprobe src/main.cpp)

if(WIN32)
    target_link_libraries(ttlprobe ws2_32)
endif()
