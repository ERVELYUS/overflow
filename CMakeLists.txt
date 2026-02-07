cmake_minimum_required(VERSION 3.10)
project(overflow VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(libs/cppcon)

# SERVER SIDE
set(SERVER_SOURCES
  src/start_server.cpp
  src/User.cpp
  src/Server.cpp
  src/Channel.cpp
)

add_executable(overflow_server ${SERVER_SOURCES})

target_include_directories(overflow_server PRIVATE include)
target_include_directories(overflow_server PRIVATE libs/cppcon/include)
target_link_libraries(overflow_server PRIVATE cppcon)

# CLIENT SIDE
set(CLIENT_SOURCES
  src/start_client.cpp
  src/Client.cpp
)

add_executable(overflow_client ${CLIENT_SOURCES})

target_include_directories(overflow_client PRIVATE include)
target_include_directories(overflow_client PRIVATE libs/cppcon/include)
target_link_libraries(overflow_client PRIVATE cppcon)
