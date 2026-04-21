#pragma once

#include <cppcon/SocketSelector.h>
#include <cppcon/TcpListener.h>

#include <string>
#include <string_view>
#include <unordered_map>

#include "Channel.h"
#include "Database.h"
#include "User.h"

class Server {
  Database m_db;
  std::unordered_map<std::string, Channel> m_channels{};

  // We have two maps for users to optimize lookup time
  std::unordered_map<socket_t, User> m_users{};
  std::unordered_map<std::string, socket_t> m_nick_to_fd{};

  TcpListener m_listener{};
  SocketSelector m_polls{};
  bool m_running{};

  bool is_valid_format(std::string_view name);
  bool is_valid_nickname(std::string_view nickname);
  bool is_valid_channel_name(std::string_view channel_name);

  std::uint32_t m_next_user_id = 1;

 public:
  Server(const std::string& ip, const std::string& port);

  void run();

  void connect_user();
  void broadcast_users_list();
  void send_dm_history(User& user);
  void handle_client_message(socket_t user_fd);

  void process_command(User& user, const Packet& packet);

  void disconnect_user(socket_t socket_fd);
  void activate_user_session(User& user, int user_id,
                             const std::string& username);

  enum class ChannelCreateReturnValue {
    SUCCESS,
    INVALID_NAME,
    ALREADY_EXISTS,
  };
  ChannelCreateReturnValue create_channel(std::string_view name);
  Channel* find_channel(std::string_view name);
};
