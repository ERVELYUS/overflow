#include "Server.h"

Server::Server(std::string port) {
  auto tcp_endpoints =
      AddrInfoResolver::resolve("127.0.0.1", port, AF_INET, SOCK_STREAM);
  if (tcp_endpoints.empty()) {
    throw std::runtime_error("Could not resolve TCP");
  }

  // Bind to a local TCP endpoint and listen on it
  m_listener.bind(tcp_endpoints[0]);
  m_listener.listen(SOMAXCONN);

  // Poll on said endpoint
  m_polls.add(m_listener, POLLIN);

  m_running = true;
}

void Server::handle_new_connection() {
  TcpSocket user_socket = m_listener.accept();
  int fd = user_socket.get_fd();
  m_polls.add(user_socket, POLLIN);
  m_users.emplace(fd, User(std::move(user_socket), "User"));
}

void Server::handle_client_message(int socket_fd) {}

void Server::process_command(User& user, const Packet& packet) {}

void Server::disconnect_user(int socket_fd) {}

Channel* Server::create_channel(std::string_view name) {}
