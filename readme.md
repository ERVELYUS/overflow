# Overflow

**Overflow** is a high-performance, cross-platform chat application built from scratch in C++17. Designed to demonstrate
low-level networking concepts and modern software architecture, Overflow features a decoupled client-server model where
a single robust networking core powers multiple distinct user interfaces: a Command Line Interface (CLI), a rich Text
User Interface (TUI), and an upcoming Graphical User Interface (GUI).

The project is architected with a strict separation between the core networking engine, the server database
architecture, and the client applications, ensuring highly modular and maintainable code.

## Powered by `cppcon`

At the heart of Overflow sits **[cppcon](https://github.com/ERVELYUS/cppcon)**, a standalone non-blocking networking
library developed specifically for this project.

`cppcon` implements the socket layer from the ground up. It handles the "dirty work" of cross-platform I/O
multiplexing—managing `poll()` on Linux and `WSAPoll()` on
Windows, abstracting header differences, and unifying error handling—so the main application code remains clean and
OS-agnostic.

## Features

### Multi-Client Architecture

* **TUI Client (`tui`):** A highly responsive, split-pane terminal interface. Powered by FTXUI.
* **CLI Client (`cli`):** A lightweight, standard I/O command-line interface for quick access and debugging.

### Application & Protocol

* **Channels & Direct Messages:** Join public community channels (`/join`) or engage in private, one-on-one direct
  messaging (`/pm`).
* **Secure Authentication:** Built-in user registration and login systems with secure password hashing via bcrypt.
* **Persistent History:** Server-side chat history, channel states, and user data are securely stored using a relational
  SQLite database.
* **Live Updates:** Real-time background pushing of active users, new channels, and incoming messages.

## Technology Stack & Credits

A huge thank you to the creators of the following tools:

* **[FTXUI](https://github.com/ArthurSonzogni/FTXUI)**: Powers the Terminal User Interface.
* **[SQLiteCpp](https://github.com/SRombauts/SQLiteCpp)**: C++ wrapper around SQLite3 for managing persistent server
  data.
* **[libbcrypt](https://github.com/trusch/libbcrypt)**: Handles secure password hashing and validation.

## Roadmap

The core networking and logic foundation is solid. The next steps focus on expanding user capabilities and fleshing out
the graphical client.

- [x] **Core Protocol & Logic:** User disconnection, `/leave`, nickname filters, DM routing, and channel creation.
- [x] **Database Integration:** SQLite schema for users, channels, and message persistence.
- [x] **TUI Implementation:** Fully interactive terminal UI client with workspaces and chat histories.
- [ ] **GUI Client:** Finalize the ImGui window implementation and API interface.
- [ ] **TUI Settings:** User-configurable settings menu within the terminal client.
- [ ] **Rich Media Support:** Rendering images and video directly in the chat buffer.
- [ ] **Voice/Video Integration:** Investigating custom UDP streams for voice and video messages.

## Build Instructions

### Prerequisites

* **CMake** (3.10+)
* **C++ Compiler** (GCC, Clang, or MSVC) supporting C++17.

### Cloning

Because Overflow utilizes several submodules, you **must** clone the repository recursively:

```bash
git clone --recursive https://github.com/ERVELYUS/overflow.git
cd overflow
```

### Building

The CMake configuration will automatically generate targets for the server and all available clients.

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Running

Start the server first. The server will automatically initialize the `server.db` SQLite database on its first run.

```bash
./server <ip> <port>
```

In separate terminal, you can launch any version of the client:

```bash
# TUI version
./tui <ip> <port>

# CLI version
./cli <ip> <port> 
```

## Contributing

1. Fork the repo
2. Create a feture branch (git checkout -b feature-name)
3. Commit your changes
4. Push to the branch and open a Pull Request
