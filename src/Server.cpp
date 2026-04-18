#include "Server.h"

#include <algorithm>
#include <iostream>

#include "Protocol.h"

Server::Server(const std::string& ip, const std::string& port)
    : m_db("server.db") {
  auto tcp_endpoints = AddrInfoResolver::resolve(ip, port);
  if (tcp_endpoints.empty()) {
    throw std::runtime_error("Could not resolve TCP");
  }

  m_db.initialize_schema();
  auto existing_channels = m_db.get_all_channels();
  for (auto& [id, name, type] : existing_channels) {
    Channel chan;
    chan.set_id(id);
    m_channels.emplace(name, std::move(chan));
  }

  // Bind to a local TCP endpoint and listen on it
  m_listener.bind(tcp_endpoints[0]);
  m_listener.listen(SOMAXCONN);

  // Poll on said endpoint
  m_polls.add(m_listener, POLLIN);

  m_running = true;
}

void Server::run() {
  m_running = true;

  std::cout << "Server created and running\n";
  while (m_running) {
    std::vector<socket_t> ready_fds = m_polls.wait();

    for (socket_t fd : ready_fds) {
      if (fd == m_listener.get_fd()) {
        connect_user();
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

  return true;
}

void Server::broadcast_users_list() {
  Packet users_packet;
  users_packet << static_cast<std::uint8_t>(CommandID::LIST_USERS)
               << static_cast<std::uint8_t>(UpdateType::BackgroundPush)
               << static_cast<std::uint32_t>(m_users.size());

  for (const auto& [fd, user] : m_users) {
    users_packet << std::string(user.get_name());
  }

  for (auto& [fd, user] : m_users) {
    user.send(users_packet);
  }
}

// TODO: Cleanup this mess
void Server::connect_user() {
  TcpSocket user_socket = m_listener.accept();
  socket_t fd = user_socket.get_fd();

  std::string default_nickname = "user_" + std::to_string(m_next_user_id++);

  m_users.emplace(fd, User(std::move(user_socket), default_nickname));
  m_nick_to_fd.emplace(default_nickname, fd);

  m_polls.add(m_users.at(fd).get_socket(), POLLIN);

  std::cout << "[LOG] New anonymous connection on FD " << fd << "\n";

  // Packet identity_packet;
  //  identity_packet << static_cast<std::uint8_t>(CommandID::SET_SELF_NAME)
  //<< default_nickname;
  //  m_users.at(fd).send(identity_packet);

  broadcast_users_list();
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

  if (!user.is_authenticated() && command != CommandID::REGISTER &&
      command != CommandID::LOGIN) {
    Packet error_packet;
    error_packet << static_cast<std::uint8_t>(CommandID::ERROR)
                 << std::string("Please register or login first.");
    user.send(error_packet);
    return;
  }

  switch (command) {
    case CommandID::REGISTER: {
      std::string username, password;
      p >> username >> password;

      Packet response;
      response << static_cast<std::uint8_t>(CommandID::REGISTER);

      if (!is_valid_format(username)) {
        response << false << std::string("Invalid username format.");
        user.send(response);
        break;
      }

      if (!m_db.register_user(username, password)) {
        response << false
                 << std::string(
                        "Username already exists or registration failed");
        std::cout << "[LOG] Unsuccessfull registration: " << username << '\n';
        user.send(response);
        break;
      }

      auto user_id = m_db.authenticate_user(username, password);
      if (!user_id.has_value()) {
        response << false
                 << std::string("Registration successful but login failed");

        std::cout << "[LOG] User registered but not logged in: " << username
                  << '\n';
        user.send(response);
        break;
      }

      activate_user_session(user, user_id.value(), username);

      response << true
               << std::string(
                      "Registration successful. You are now logged in as " +
                      username + ".");
      std::cout << "[LOG] User registered and logged in: " << username << '\n';
      user.send(response);
      break;
    }
    case CommandID::LOGIN: {
      std::string username, password;
      p >> username >> password;

      Packet response;
      response << static_cast<std::uint8_t>(CommandID::LOGIN);

      auto user_id = m_db.authenticate_user(username, password);
      if (!user_id.has_value()) {
        response << false << std::string("Invalid username or password.");
        std::cout << "[LOG] Failed login attempt: " << username << '\n';
        user.send(response);
        break;
      }

      activate_user_session(user, user_id.value(), username);

      response << true << username;
      std::cout << "[LOG] User authenticated and renamed: " << username << '\n';
      user.send(response);
      break;
    }
    case CommandID::NICKNAME: {
      std::string new_name;
      p >> new_name;

      Packet nickname_change_msg{};
      nickname_change_msg << static_cast<std::uint8_t>(CommandID::NICKNAME);
      if (!is_valid_nickname(new_name)) {
        nickname_change_msg << false;
        user.send(nickname_change_msg);
        std::cout << "[LOG] User " << user.get_socket().get_fd()
                  << " tried to change nickname unsuccessfully\n"
                  << std::flush;
        break;
      }
      if (new_name == user.get_name()) {
        nickname_change_msg << true << new_name;
        user.send(nickname_change_msg);
        break;
      }

      try {
        m_db.update_username(user.get_id(), new_name);

        // Sync
        m_nick_to_fd.erase(std::string(user.get_name()));
        m_nick_to_fd.emplace(new_name, user.get_socket().get_fd());
        user.set_name(new_name);

        nickname_change_msg << true << new_name;
        std::cout << "[LOG] Renaming User on FD " << user.get_socket().get_fd()
                  << " from '" << user.get_name() << "' to '" << new_name
                  << "'\n"
                  << std::flush;
        broadcast_users_list();
      }
      catch (...) {
        nickname_change_msg << false;
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

        m_db.add_user_to_channel(target_channel->get_id(), user.get_id());
        auto history = m_db.get_recent_messages(target_channel->get_id(), 50);
        for (const auto& msg : history) {
          Packet hist_packet;
          hist_packet << static_cast<std::uint8_t>(CommandID::MSG)
                      << msg.sender_name << msg.content;
          user.send(hist_packet);
        }
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

        m_db.save_message(channel.get_id(), user.get_id(), message_text);

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

      int target_db_id = -1;
      User* target_user_obj = nullptr;

      if (it != m_nick_to_fd.end()) {
        target_user_obj = &m_users.at(it->second);
        target_db_id = target_user_obj->get_id();
      }
      else {
        // TODO: Dm to offline users
      }

      if (target_db_id != -1) {
        int dm_channel_id = m_db.get_or_create_dm(user.get_id(), target_db_id);

        m_db.save_message(dm_channel_id, user.get_id(), message_text);

        if (target_user_obj) {
          Packet dm_packet{};
          dm_packet << static_cast<std::uint8_t>(CommandID::PRIVATE_MSG)
                    << std::string(user.get_name()) << message_text;
          target_user_obj->send(dm_packet);
        }

        std::cout << "[LOG] DM saved and sent from @" << user.get_name()
                  << " to @" << target_name << '\n';
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
        m_db.remove_user_from_channel(it->second.get_id(), user.get_id());
      }
      std::cout << "[LOG] User " << user.get_name() << " left #"
                << target_channel << " channel\n";
      break;
    }
    case CommandID::LIST_CHANNELS: {
      Packet channels_list{};
      channels_list << static_cast<std::uint8_t>(CommandID::LIST_CHANNELS)
                    << static_cast<std::uint8_t>(UpdateType::ManualRequest)
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
      std::uint32_t count = 0;
      if (m_users.size() > 0) {
        count = static_cast<std::uint32_t>(m_users.size() - 1);
      }
      users_list << static_cast<std::uint8_t>(CommandID::LIST_USERS)
                 << static_cast<std::uint8_t>(UpdateType::ManualRequest)
                 << count;
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
        std::string success_message = "Channel #" + channel_name + " created.";
        create_result << success_message;
        std::cout << "[LOG] User " << user.get_name() << " created a channel #"
                  << channel_name << '\n';

        Packet update_packet;
        update_packet << static_cast<std::uint8_t>(CommandID::LIST_CHANNELS)
                      << static_cast<std::uint8_t>(UpdateType::BackgroundPush)
                      << static_cast<std::uint32_t>(m_channels.size());
        for (auto const& [name, chan] : m_channels) {
          update_packet << name;
        }
        for (auto& [fd, user] : m_users) {
          user.send(update_packet);
        }
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

  broadcast_users_list();
}

void Server::activate_user_session(User& user, int user_id,
                                   const std::string& username) {
  user.authenticate();
  user.set_id(user_id);

  std::string old_name(user.get_name());
  m_nick_to_fd.erase(old_name);
  m_nick_to_fd.emplace(username, user.get_socket().get_fd());
  user.set_name(username);

  broadcast_users_list();
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

  int db_id = m_db.get_or_create_channel(std::string(name));
  Channel new_channel;
  new_channel.set_id(db_id);
  m_channels.emplace(std::string(name), std::move(new_channel));
  return ChannelCreateReturnValue::SUCCESS;
}
