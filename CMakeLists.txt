cmake_minimum_required(VERSION 3.10)
project(overflow VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Subprojects / External libs
add_subdirectory(libs/cppcon)
add_subdirectory(libs/ftxui)
add_subdirectory(libs/guiHelper)

set(CPPCON_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/cppcon/include")

# Server
add_executable(overflow_server
    src/start_server.cpp
    src/User.cpp
    src/Server.cpp
    src/Channel.cpp
)

target_include_directories(overflow_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CPPCON_INCLUDE_DIR}
)

target_link_libraries(overflow_server PRIVATE
    cppcon
)

# CLI Client
add_executable(overflow_client
    src/start_client.cpp
    src/Client.cpp
)

target_include_directories(overflow_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CPPCON_INCLUDE_DIR}
)

target_link_libraries(overflow_client PRIVATE
    cppcon
)

# GUI Client
add_executable(overflow_gui_client
    src/start_gui_client.cpp
    src/Client.cpp
)

target_include_directories(overflow_gui_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CPPCON_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/guiHelper/include
)

target_link_libraries(overflow_gui_client PRIVATE
    cppcon
    guiHelper
)

# TUI Client
add_executable(overflow_tui_client
    src/start_tui_client.cpp
    src/TuiApp.cpp
    src/tui/Layouts.cpp
    src/Client.cpp
)

target_include_directories(overflow_tui_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CPPCON_INCLUDE_DIR}
)

target_link_libraries(overflow_tui_client PRIVATE
    cppcon
    ftxui::screen
    ftxui::dom
    ftxui::component
)

# Platform-specific Linking
if(UNIX)
  find_package(Threads REQUIRED)

  target_link_libraries(overflow_server PRIVATE Threads::Threads)
  target_link_libraries(overflow_client PRIVATE Threads::Threads)
  target_link_libraries(overflow_gui_client PRIVATE Threads::Threads)
  target_link_libraries(overflow_tui_client PRIVATE Threads::Threads)
endif()

if(WIN32)
  target_link_libraries(overflow_server PRIVATE Ws2_32)
  target_link_libraries(overflow_client PRIVATE Ws2_32)
  target_link_libraries(overflow_gui_client PRIVATE Ws2_32)
  target_link_libraries(overflow_tui_client PRIVATE Ws2_32)
endif()
