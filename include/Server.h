#pragma once

#include <cppcon/SocketSelector.h>
#include <cppcon/TcpListener.h>
#include <cppcon/TcpSocket.h>

#include <string>
#include <string_view>
#include <unordered_map>

#include "Channel.h"
#include "User.h"

class Server {
  // TODO: users are defined as int (file descriptor) and User object,
  // meanwhile channels are defined as string (channel name) and Channel object
  // which maybe creates a layer of inconsistency?
  std::unordered_map<socket_t, User> m_users{};
  std::unordered_map<std::string, Channel> m_channels{};

  TcpListener m_listener{};
  SocketSelector m_polls{};
  bool m_running{};

 public:
  Server(const std::string& ip, const std::string& port);

  void run();

  void handle_new_connection();
  void handle_client_message(socket_t user_fd);

  void process_command(User& user, const Packet& packet);

  void disconnect_user(socket_t socket_fd);

  Channel* create_channel(std::string_view name);
};
