cmake_minimum_required(VERSION 3.10)
project(overflow VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Disabling stuff inside SQLiteCpp
set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "" FORCE)
set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "" FORCE)
set(SQLITECPP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SQLITECPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)

# Subprojects / External libs
add_subdirectory(libs/cppcon)
add_subdirectory(libs/ftxui)
add_subdirectory(libs/guiHelper)
add_subdirectory(libs/SQLiteCpp)
add_subdirectory(libs/libbcrypt)

set(CPPCON_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/cppcon/include")

# Shared Client Logic (used by CLI, TUI, and GUI)
set(CLIENT_COMMON_SOURCES
        src/Client.cpp
        src/Console.cpp
)

# Server
add_executable(server
        src/start_server.cpp
        src/User.cpp
        src/Server.cpp
        src/Channel.cpp
        src/Database.cpp
)

target_include_directories(server PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CPPCON_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/SQLiteCpp/include
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/libbcrypt/include
)

target_link_libraries(server PRIVATE
        cppcon
        SQLiteCpp
        sqlite3
        bcrypt
)

# CLI Client
add_executable(cli
        src/start_cli_client.cpp
        src/CliApp.cpp
        ${CLIENT_COMMON_SOURCES}
)

target_include_directories(cli PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CPPCON_INCLUDE_DIR}
)

target_link_libraries(cli PRIVATE
        cppcon
)

# GUI Client
add_executable(gui
        src/start_gui_client.cpp
        ${CLIENT_COMMON_SOURCES}
)

target_include_directories(gui PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CPPCON_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/guiHelper/include
)

target_link_libraries(gui PRIVATE
        cppcon
        guiHelper
)

# TUI Client
add_executable(tui
        src/start_tui_client.cpp
        src/tui/TuiApp_init.cpp
        src/tui/TuiApp_messages.cpp
        src/tui/TuiApp_auth.cpp
        src/tui/TuiApp_chat.cpp
        src/tui/TuiApp_layout.cpp
        ${CLIENT_COMMON_SOURCES}
)

target_include_directories(tui PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CPPCON_INCLUDE_DIR}
)

target_link_libraries(tui PRIVATE
        cppcon
        ftxui::screen
        ftxui::dom
        ftxui::component
)

# Platform-specific Linking
if (UNIX)
    find_package(Threads REQUIRED)

    set(THREADS_LIBS Threads::Threads)
    target_link_libraries(server PRIVATE ${THREADS_LIBS})
    target_link_libraries(cli PRIVATE ${THREADS_LIBS})
    target_link_libraries(gui PRIVATE ${THREADS_LIBS})
    target_link_libraries(tui PRIVATE ${THREADS_LIBS})
endif ()

if (WIN32)
    set(WINSOCK_LIB Ws2_32)
    target_link_libraries(server PRIVATE ${WINSOCK_LIB})
    target_link_libraries(cli PRIVATE ${WINSOCK_LIB})
    target_link_libraries(gui PRIVATE ${WINSOCK_LIB})
    target_link_libraries(tui PRIVATE ${WINSOCK_LIB})
endif ()
