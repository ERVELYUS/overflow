# Overflow 

**Overflow** is a high-performance, cross-platform chat application built from scratch in C++. It demonstrates low-level networking concepts, non-blocking I/O, and modern C++17 design patterns.

The project is architected with a strict separation between the core networking engine (**cppcon**) and the application logic (Server/Client), ensuring modularity and clean code.

## Features Implemented

### Core Networking (via `cppcon`)
* **Cross-Platform Architecture:** Seamlessly compiles and runs on **Linux** (GCC/Clang) and **Windows** (MSVC/MinGW).
* **Custom Socket Wrappers:** RAII-compliant `TcpSocket`, `TcpListener`, and `UdpSocket` classes that handle OS-specific resource management automatically.
* **I/O Multiplexing:** High-concurrency event loops using `poll()` on Linux and `WSAPoll()` on Windows to handle multiple clients on a single thread.
* **Platform Abstraction:** Unified types (`socket_t`, `nfds_t`) and error handling (`WSAGetLastError` vs `errno`) hidden behind a clean API.

### Application Layer
* **Client-Server Model:** A dedicated Server executable that manages connections and a Client executable for users.
* **Domain Logic:** `User` and `Channel` classes implemented for state management foundation.
* **Build System:** Modern CMake configuration with submodules 

## The Engine

This project is powered by **[cppcon](https://github.com/ERVELYUS/cppcon)**, a standalone networking library developed specifically for Overflow. 

Instead of relying on heavy external frameworks like Boost.Asio, `cppcon` implements the socket layer from the ground up. It handles the "dirty work" of cross-platform compatibility—managing `WSAStartup` on Windows, header differences, and type mismatches—so the main application code remains clean and OS-agnostic.


## Roadmap & To-Do

The foundation is solid. The next steps focus on **Protocol Robustness, Logic, and UI**.

### 1. Server Logic 
- [X] **User Disconnection:** Handle user disconnecting from the server
- [X] **User Leavng Channel:** Handle user typing `/leave`
- [X] **Nickname filters:** Add filters to `/nick <name>` to restrict usernames
- [X] **Optimize `SocketSelector`:** Change SocketSelector behaviour to return *changed* sockets (`cppcon` issue)
- [X] **Graceful channel creation:** Give the user ability to create channels via `/create <channel_name>`
- [X] **64 bit support on Windows:** Figure out a way to add a 64 bit support on Windows
- [X] **Direct contact:** Add `/msg` as a way to send DMs

### 2. Features
- [ ] **UI:** Create a UI version via imgui

## Build Instructions

### Prerequisites
* **CMake** (3.10+)
* **C++ Compiler** (GCC, Clang, or MSVC) supporting C++17.

### Cloning
Since `cppcon` is a submodule, you must clone recursively:

```bash
git clone --recursive https://github.com/ERVELYUS/overflow.git
cd overflow
```
*(If you already cloned without recursive, run git submodule update --init --recursive)*

### Building 
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Running

Start the server first, then connect with one or more clients:

```bash
# Terminal 1
./overflow_server

# Terminal 2
./overflow_client
```

## Contributing
1. Fork the repository.
2. Create a feature branch (git checkout -b feature-name).
3. Commit your changes (git commit -m "Add feature").
4. Push to the branch (git push origin feature-name).
5. Open a Pull Request.
