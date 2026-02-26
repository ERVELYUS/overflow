cmake_minimum_required(VERSION 3.10)
project(overflow VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

include_directories(overflow PRIVATE include)
include_directories(overflow PRIVATE libs/cppcon/include)
add_subdirectory(libs/cppcon)
add_subdirectory(libs/ftxui)
add_subdirectory(libs/guiHelper)

# SERVER SIDE
set(SERVER_SOURCES
  src/start_server.cpp
  src/User.cpp
  src/Server.cpp
  src/Channel.cpp
)

add_executable(overflow_server ${SERVER_SOURCES})
target_link_libraries(overflow_server PRIVATE cppcon)

# CLIENT SIDE
set(CLIENT_SOURCES
  src/start_client.cpp
  src/Client.cpp
)

add_executable(overflow_client ${CLIENT_SOURCES})
target_link_libraries(overflow_client PRIVATE cppcon)


# GUI CLIENT SIDE
set(GUI_CLIENT_SOURCES
  src/start_gui_client.cpp
  src/Client.cpp
)

add_executable(overflow_gui_client ${GUI_CLIENT_SOURCES})
target_include_directories(overflow_gui_client PRIVATE libs/guiHelper/include)
target_link_libraries(overflow_gui_client PRIVATE cppcon guiHelper)

# TUI CLIENT SIDE
set(TUI_CLIENT_SOURCES
  src/start_tui_client.cpp
  src/TuiApp.cpp
  src/Client.cpp
)

add_executable(overflow_tui_client ${TUI_CLIENT_SOURCES})
target_include_directories(overflow_tui_client PRIVATE include)
target_link_libraries(overflow_tui_client PRIVATE
    cppcon
    ftxui::screen
    ftxui::dom
    ftxui::component
)

if(UNIX)
  target_link_libraries(overflow_tui_client PRIVATE pthread)
endif()

if(WIN32)
  target_link_libraries(overflow_server PRIVATE Ws2_32)
  target_link_libraries(overflow_client PRIVATE Ws2_32)
  target_link_libraries(overflow_gui_client PRIVATE Ws2_32)
  target_link_libraries(overflow_tui_client PRIVATE Ws2_32)
endif()
