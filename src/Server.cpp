#include "Server.h"

#include <algorithm>
#include <iostream>

#include "Protocol.h"

void Server::run() {
  m_running = true;

  std::cout << "Server created and running\n";
  while (m_running) {
    std::vector<socket_t> ready_fds = m_polls.wait();

    for (socket_t fd : ready_fds) {
      if (fd == m_listener.get_fd()) {
        handle_new_connection();
      }
      else {
        if (m_users.find(fd) != m_users.end()) {
          handle_client_message(fd);
        }
      }
    }
  }
}

bool Server::is_valid_format(std::string_view name) {
  // Length check
  if (name.length() < 3 || name.length() > 20) {
    return false;
  }

  // Valid chars check
  auto is_valid_char = [](char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
  };
  if (!std::all_of(name.begin(), name.end(), is_valid_char)) {
    return false;
  }

  // Reserved names (might be expanded)
  std::string lower_name{};
  for (char c : name) {
    lower_name += std::tolower(static_cast<unsigned char>(c));
  }
  if (lower_name == "system" || lower_name == "admin" || lower_name == "root" ||
      lower_name == "server" || lower_name == "log") {
    return false;
  }

  return true;
}

bool Server::is_valid_nickname(std::string_view nickname) {
  // Check for correct format
  if (!is_valid_format(nickname)) {
    return false;
  }

  // Uniqueness check
  return m_nick_to_fd.find(std::string(nickname)) == m_nick_to_fd.end();
}

bool Server::is_valid_channel_name(std::string_view channel_name) {
  // Check for correct format
  if (!is_valid_format(channel_name)) {
    return false;
  }

  // Uniqueness check (made redundant by create_channel)
  /* if (m_channels.find(std::string(channel_name)) != m_channels.end()) {
    return false;
  } */

  return true;
}

Server::Server(const std::string& ip, const std::string& port) {
  auto tcp_endpoints = AddrInfoResolver::resolve(ip, port);
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
  socket_t fd = user_socket.get_fd();
  m_polls.add(user_socket, POLLIN);
  std::string default_nickname = "user_" + std::to_string(m_next_user_id++);
  m_users.emplace(fd, User(std::move(user_socket), default_nickname));
  m_nick_to_fd.emplace(default_nickname, fd);
  std::cout << "[LOG] New user connected\n" << std::flush;
}

void Server::handle_client_message(socket_t user_fd) {
  auto& user = m_users.at(user_fd);

  Packet user_message{};
  if (!user.recv(user_message)) {
    disconnect_user(user_fd);
  }
  else {
    process_command(user, user_message);
  }
}

void Server::process_command(User& user, const Packet& packet) {
  Packet p = packet;

  std::uint8_t command_protocol;
  p >> command_protocol;

  CommandID command = static_cast<CommandID>(command_protocol);

  switch (command) {
    case CommandID::NICKNAME: {
      std::string new_name;
      p >> new_name;

      Packet nickname_change_msg{};
      if (is_valid_nickname(new_name)) {
        nickname_change_msg << static_cast<std::uint8_t>(CommandID::NICKNAME)
                            << true << new_name;
        std::cout << "[LOG] Renaming User on FD " << user.get_socket().get_fd()
                  << " from '" << user.get_name() << "' to '" << new_name
                  << "'\n"
                  << std::flush;

        // Sync
        m_nick_to_fd.erase(std::string(user.get_name()));
        m_nick_to_fd.emplace(new_name, user.get_socket().get_fd());

        user.set_name(new_name);
      }
      else {
        nickname_change_msg << static_cast<std::uint8_t>(CommandID::NICKNAME)
                            << false;
        std::cout << "[LOG] User " << user.get_socket().get_fd()
                  << " tried to change nickname unsuccessfully\n"
                  << std::flush;
      }
      user.send(nickname_change_msg);
      break;
    }
    case CommandID::JOIN: {
      std::string channel_name;
      p >> channel_name;

      Channel* target_channel = find_channel(channel_name);

      if (target_channel != nullptr) {
        target_channel->add_user(user.get_socket().get_fd());
        Packet success_packet;
        success_packet << static_cast<std::uint8_t>(CommandID::JOIN) << true
                       << channel_name;
        user.send(success_packet);
        std::cout << "[LOG] User " << user.get_name() << " joined #"
                  << channel_name << " channel\n";
      }
      else {
        Packet error_packet;
        error_packet << static_cast<std::uint8_t>(CommandID::JOIN) << false;
        user.send(error_packet);
        std::cout << "[LOG] User " << user.get_name() << " failed to join #"
                  << channel_name << " (Not found)\n";
      }

      break;
    }
    case CommandID::MSG: {
      std::string target_channel, message_text;
      p >> target_channel >> message_text;

      auto it = m_channels.find(target_channel);
      if (it != m_channels.end()) {
        Channel& channel = it->second;

        Packet broadcast_packet;
        broadcast_packet << static_cast<std::uint8_t>(CommandID::MSG)
                         << std::string(user.get_name()) << message_text;

        for (socket_t target_fd : channel.get_users()) {
          if (target_fd == user.get_socket().get_fd()) continue;

          m_users.at(target_fd).send(broadcast_packet);
        }
      }
      break;
    }
    case CommandID::PRIVATE_MSG: {
      std::string target_name, message_text;
      p >> target_name >> message_text;

      auto it = m_nick_to_fd.find(target_name);
      if (it != m_nick_to_fd.end()) {
        socket_t target_fd = it->second;

        auto user_it = m_users.find(target_fd);
        if (user_it != m_users.end()) {
          Packet dm_packet{};
          dm_packet << static_cast<std::uint8_t>(CommandID::PRIVATE_MSG)
                    << std::string(user.get_name()) << message_text;

          user_it->second.send(dm_packet);

          std::cout << "[LOG] User @" << user.get_name() << " sent DM to user @"
                    << target_name << '\n';
        }
        else {
          m_nick_to_fd.erase(it);
        }
      }
      else {
        Packet error_packet{};
        error_packet << static_cast<std::uint8_t>(CommandID::ERROR)
                     << "User @" + target_name +
                            " is offline or does not exist.";
        user.send(error_packet);
      }
      break;
    }
    case CommandID::LEAVE: {
      std::string target_channel{};
      p >> target_channel;

      auto it = m_channels.find(target_channel);
      if (it != m_channels.end()) {
        it->second.remove_user(user.get_socket().get_fd());
      }
      std::cout << "[LOG] User " << user.get_name() << " left #"
                << target_channel << " channel\n";
      break;
    }
    case CommandID::LIST_CHANNELS: {
      Packet channels_list{};
      channels_list << static_cast<std::uint8_t>(CommandID::LIST_CHANNELS)
                    << static_cast<std::uint32_t>(m_channels.size());
      for (auto channel : m_channels) {
        channels_list << channel.first;
      }
      std::cout << "[LOG] User " << user.get_name()
                << " requested a list of channels\n"
                << std::flush;
      user.send(channels_list);

      break;
    }
    case CommandID::LIST_USERS: {
      Packet users_list{};
      users_list << static_cast<std::uint8_t>(CommandID::LIST_USERS)
                 << static_cast<std::uint32_t>(m_users.size()) - 1;
      for (const auto& [fd, online_user] : m_users) {
        if (user.get_name() == online_user.get_name()) {
          continue;
        }
        users_list << std::string(online_user.get_name());
      }
      user.send(users_list);

      break;
    }
    case CommandID::CREATE: {
      std::string channel_name{};
      p >> channel_name;

      Packet create_result{};
      create_result << static_cast<std::uint8_t>(CommandID::CREATE);
      ChannelCreateReturnValue result = create_channel(channel_name);
      if (result == ChannelCreateReturnValue::SUCCESS) {
        std::string success_message =
            "[System] Channel #" + channel_name + " created.\n";
        create_result << success_message;
        std::cout << "[LOG] User " << user.get_name() << " created a channel #"
                  << channel_name << '\n';
      }
      else {
        std::string error_message;
        if (result == ChannelCreateReturnValue::INVALID_NAME) {
          error_message +=
              "[System] Invalid channel name. Use 3-20 alphanumeric chars.\n";
        }
        else if (result == ChannelCreateReturnValue::ALREADY_EXISTS) {
          error_message += "[System] Channel with that name already exists.\n";
        }
        create_result << error_message;
        std::cout << "[LOG] User " << user.get_name()
                  << " tried to create a channel #" << channel_name
                  << " unsuccessfully\n";
      }

      user.send(create_result);
      break;
    }
    case CommandID::ERROR:
    case CommandID::NONE:
    default: {
      std::cerr << "Received invalid or unexpected command ID from user "
                << user.get_name() << '\n';
      break;
    }
  }
}

void Server::disconnect_user(socket_t user_fd) {
  auto it = m_users.find(user_fd);
  if (it == m_users.end()) return;

  User& user_to_delete = it->second;

  m_nick_to_fd.erase(std::string(user_to_delete.get_name()));

  for (auto& [name, channel] : m_channels) {
    channel.remove_user(user_fd);
  }

  m_polls.remove(user_to_delete.get_socket());
  m_users.erase(it);

  std::cout << "[LOG] Socket " << user_fd << " disconnected and cleaned up\n";
}

Channel* Server::find_channel(std::string_view name) {
  auto it = m_channels.find(std::string(name));

  if (it != m_channels.end()) {
    return &(it->second);
  }

  return nullptr;
}

Server::ChannelCreateReturnValue Server::create_channel(std::string_view name) {
  if (!is_valid_channel_name(name)) {
    return ChannelCreateReturnValue::INVALID_NAME;
  }
  if (m_channels.find(std::string(name)) != m_channels.end()) {
    return ChannelCreateReturnValue::ALREADY_EXISTS;
  }

  m_channels.emplace(std::string(name), Channel());
  return ChannelCreateReturnValue::SUCCESS;
}
