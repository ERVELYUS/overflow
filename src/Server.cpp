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
