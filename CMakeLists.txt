cmake_minimum_required(VERSION 3.10)
project(overflow VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(libs/cppcon)

set(SOURCE_FILES
  src/start_server.cpp
  src/User.cpp
  src/Server.cpp
  src/Channel.cpp
)

add_executable(overflow ${SOURCE_FILES})

target_link_libraries(overflow PRIVATE cppcon)
target_include_directories(overflow PRIVATE include)
target_include_directories(overflow PRIVATE libs/cppcon/include)
