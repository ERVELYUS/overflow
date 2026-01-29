#pragma once

#include <cppcon/SocketSelector.h>
#include <cppcon/TcpListener.h>
#include <cppcon/TcpSocket.h>

#include <string>
#include <string_view>
#include <unordered_map>

#include "Channel.h"
#include "Protocol.h"
#include "User.h"

class Server {
  std::unordered_map<int, User> m_users{};
  std::unordered_map<std::string, Channel> m_channels{};

  TcpListener m_listener{};
  SocketSelector m_polls{};
  bool m_running{};

 public:
  Server(std::string port);

  void run();

  void handle_new_connection();
  void handle_client_message(int socket_fd);

  void process_command(User& user, const Packet& packet);

  void disconnect_user(int socket_fd);

  Channel* create_channel(std::string_view name);
};
