#include "Server.h"

#include <iostream>

#include "Protocol.h"

void Server::run() {
  m_running = true;

  while (m_running) {
    m_polls.wait();

    if (m_polls.is_ready(m_listener)) {
      handle_new_connection();
    }

    // TODO change SocketSelector to return list of sockets that changed
    auto it = m_users.begin();
    while (it != m_users.end()) {
      auto current = it++;
      if (m_polls.is_ready(current->second.get_socket())) {
        handle_client_message(current->first);
      }
    }
  }
}

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
  m_users.emplace(fd, User(std::move(user_socket), "user"));
}

void Server::handle_client_message(int user_fd) {
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
    case CommandID::Nickname: {
      std::string new_name;
      p >> new_name;
      user.set_name(new_name);
      break;
    }
    case CommandID::Join: {
      std::string channel_name;
      p >> channel_name;

      // TODO maybe not allow user to create random channels?
      Channel* channel = create_channel(channel_name);

      channel->add_user(&user);
      break;
    }
    case CommandID::Msg: {
      std::string target_channel;
      std::string message_text;

      p >> target_channel >> message_text;

      auto it = m_channels.find(target_channel);
      if (it != m_channels.end()) {
        Channel& channel = it->second;

        Packet broadcast_packet;
        broadcast_packet << std::uint8_t(CommandID::Msg)
                         << std::string(user.get_name()) << message_text;

        channel.broadcast(broadcast_packet, &user);
      }
      break;
    }
    case CommandID::Leave: {
      std::string target_channel;
      p >> target_channel;

      auto it = m_channels.find(target_channel);
      if (it != m_channels.end()) {
        it->second.remove_user(&user);
      }
      break;
    }
    case CommandID::Error:
    case CommandID::None:
    default: {
      std::cerr << "Received invalid or unexpected command ID from user "
                << user.get_name() << '\n';
      break;
    }
  }
}

void Server::disconnect_user(int user_fd) {
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
