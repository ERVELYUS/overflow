#include "Server.h"

#include <algorithm>
#include <iostream>

#include "Protocol.h"

void Server::run() {
  m_running = true;

  std::cout << "Server created and running\n";
  while (m_running) {
    m_polls.wait();

    if (m_polls.is_ready(m_listener)) {
      handle_new_connection();
    }

    // TODO: change SocketSelector to return list of sockets that changed
    auto it = m_users.begin();
    while (it != m_users.end()) {
      auto current = it++;
      if (m_polls.is_ready(current->second.get_socket())) {
        handle_client_message(current->first);
      }
    }
  }
}

bool Server::is_valid_nickname(std::string_view nickname) {
  // Length check
  if (nickname.length() < 3 || nickname.length() > 20) {
    return false;
  }

  // Valid chars check
  auto is_valid_char = [](char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
  };
  if (!std::all_of(nickname.begin(), nickname.end(), is_valid_char)) {
    return false;
  }

  // Reserved names (might be expanded)
  std::string check_name{};
  std::for_each(nickname.begin(), nickname.end(),
                [](char c) { c = tolower(c); });
  if (check_name == "system" || check_name == "admin" || check_name == "root" ||
      check_name == "server") {
    return false;
  }

  // Uniqueness check
  for (const auto& [fd, user] : m_users) {
    if (user.get_name() == nickname) {
      return false;
    }
  }

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
  m_users.emplace(fd, User(std::move(user_socket), "user"));
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

      // TODO: maybe not allow user to create random channels?
      Channel* channel = create_channel(channel_name);

      std::cout << "[LOG] User " << user.get_name() << " joined #"
                << channel_name << " channel\n";
      channel->add_user(&user);
      break;
    }
    case CommandID::MSG: {
      std::string target_channel;
      std::string message_text;

      p >> target_channel >> message_text;

      auto it = m_channels.find(target_channel);
      if (it != m_channels.end()) {
        Channel& channel = it->second;

        Packet broadcast_packet;
        broadcast_packet << static_cast<std::uint8_t>(CommandID::MSG)
                         << std::string(user.get_name()) << message_text;

        channel.broadcast(broadcast_packet, &user);
      }
      break;
    }
    case CommandID::LEAVE: {
      std::string target_channel;
      p >> target_channel;

      auto it = m_channels.find(target_channel);
      if (it != m_channels.end()) {
        it->second.remove_user(&user);
      }
      std::cout << "[LOG] User " << user.get_name() << " left #"
                << target_channel << " channel\n";
      break;
    }
    case CommandID::LIST: {
      Packet channels_list{};
      channels_list << static_cast<std::uint8_t>(CommandID::LIST)
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
  User* user_to_delete = &m_users.at(user_fd);
  for (auto& channel : m_channels) {
    channel.second.remove_user(user_to_delete);
  }
  m_polls.remove(user_to_delete->get_socket());

  m_users.erase(user_fd);
}

Channel* Server::create_channel(std::string_view name) {
  // Because it's a map, even if channel doesn't exist - it will create one
  return &m_channels[std::string(name)];
}
