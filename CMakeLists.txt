cmake_minimum_required(VERSION 3.10)
project(overflow VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(libs/cppcon)

set(SOURCE_FILES
  src/main.cpp
  src/User.cpp
)

add_executable(overflow ${SOURCE_FILES})

target_link_libraries(overflow PRIVATE cppcon)
target_include_directories(overflow PRIVATE include)
target_include_directories(overflow PRIVATE libs/cppcon/include)

