#include "Client.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>

#include "Console.h"
#include "Message.h"
#include "Protocol.h"
#include "cppcon/AddrInfoResolver.h"

Client::Client() : m_running(false) {}

Client::~Client() {
  m_running = false;

  m_socket.close();

  if (m_recieve_thread.joinable()) {
    m_recieve_thread.join();
  }
}

void Client::connect(const std::string& ip, const std::string& port) {
  auto tcp_endpoints = AddrInfoResolver::resolve(ip, port);
  if (tcp_endpoints.empty()) {
    throw std::runtime_error("Could not resolve server address");
  }

  m_socket.connect(tcp_endpoints[0]);
  m_connected = true;
  m_running = true;

  m_recieve_thread = std::thread([this]() {
    while (m_running) {
      Packet packet{};

      if (m_socket.recv(packet)) {
        handle_server_message(packet);
      }
      else {
        if (m_running) {
          // Console::print(ConsoleLevel::System, "Disconnected from server.");
          m_running = false;
        }
        break;
      }
    }
  });
}

void Client::handle_server_message(Packet& packet) {
  std::uint8_t id_raw{};
  packet >> id_raw;
  CommandID id = static_cast<CommandID>(id_raw);

  if (id == CommandID::MSG) {
    std::string sender, content;
    packet >> sender >> content;

    // Here's the the handler hook
    auto message = std::make_shared<UserMessage>(UserMessage(sender, content));
    if (m_message_handler) {
      m_message_handler(message);
    }

    Console::print(ConsoleLevel::Info, "[" + sender + "] " + content);
    Console::print(ConsoleLevel::Prompt, "> ");
  }
  else if (id == CommandID::SET_SELF_NAME) {
    std::string new_name{};
    packet >> new_name;

    m_nickname = new_name;
    auto msg = std::make_shared<SelfNameMessage>(new_name);
    if (m_message_handler) {
      m_message_handler(msg);
    }
  }
  else if (id == CommandID::LIST_CHANNELS) {
    std::uint8_t update_raw{};
    packet >> update_raw;
    UpdateType type = static_cast<UpdateType>(update_raw);

    if (type == UpdateType::ManualRequest) {
      Console::print(ConsoleLevel::System, "List of all available channels:");
    }

    std::uint32_t channels_count{};
    packet >> channels_count;

    std::string channel_name;

    ChannelsList channelNames;

    for (std::uint32_t i = 0; i < channels_count; ++i) {
      packet >> channel_name;
      channelNames << channel_name;
      if (type == UpdateType::ManualRequest) {
        Console::print(ConsoleLevel::Info, "#" + channel_name);
      }
    }

    auto message = std::make_shared<ChannelsList>(channelNames);
    if (m_message_handler) m_message_handler(message);

    if (type == UpdateType::ManualRequest) {
      Console::print(ConsoleLevel::Prompt, "> ");
    }
  }
  else if (id == CommandID::LIST_USERS) {
    std::uint8_t update_raw{};
    packet >> update_raw;
    UpdateType type = static_cast<UpdateType>(update_raw);

    if (type == UpdateType::ManualRequest) {
      Console::print(ConsoleLevel::System, "List of all available users:");
    }
    std::uint32_t users_count{};
    packet >> users_count;

    std::string user_name;

    UsersList usernames;
    for (std::uint32_t i = 0; i < users_count; ++i) {
      packet >> user_name;
      usernames << user_name;
      if (type == UpdateType::ManualRequest) {
        Console::print(ConsoleLevel::Info, "@" + user_name);
      }
    }
    auto message = std::make_shared<UsersList>(usernames);
    if (m_message_handler) m_message_handler(message);

    if (type == UpdateType::ManualRequest) {
      Console::print(ConsoleLevel::Prompt, "> ");
    }
  }
  else if (id == CommandID::NICKNAME) {
    bool successful{};
    packet >> successful;
    if (successful) {
      std::string confirmed_name;
      packet >> confirmed_name;
      m_nickname = confirmed_name;
      auto msg = std::make_shared<SelfNameMessage>(confirmed_name);
      if (m_message_handler) m_message_handler(msg);
      Console::print(ConsoleLevel::System, "Name changed successfully.");
    }
    else {
      Console::print(ConsoleLevel::System,
                     "Invalid nickname. Use 3-20 alphanumeric characters.");
    }

    Console::print(ConsoleLevel::Prompt, "> ");
  }
  else if (id == CommandID::JOIN) {
    bool successful{};
    packet >> successful;
    if (successful) {
      std::string channel_name;
      packet >> channel_name;
      m_current_channel = channel_name;
      Console::print(ConsoleLevel::System,
                     "Connected to channel #" + channel_name);
    }
    else {
      Console::print(ConsoleLevel::System,
                     "Channel does not exist. Use /create to create it");
    }

    Console::print(ConsoleLevel::Prompt, "> ");
  }
  else if (id == CommandID::PRIVATE_MSG) {
    std::string sender, content;
    packet >> sender >> content;
    Console::print(ConsoleLevel::Info, "[@" + sender + "]: " + content);
    Console::print(ConsoleLevel::Prompt, "> ");
  }
  else if (id == CommandID::CREATE) {
    std::string message{};
    packet >> message;

    Console::print(ConsoleLevel::System, message);
    Console::print(ConsoleLevel::Prompt, "> ");
  }
  else if (id == CommandID::ERROR) {
    std::string error_msg;
    packet >> error_msg;
    Console::print(ConsoleLevel::Error, error_msg);
    Console::print(ConsoleLevel::Prompt, "> ");
  }
}

void Client::run() {
  std::string line{};
  Console::print(ConsoleLevel::Prompt, "> ");

  while (m_running && std::getline(std::cin, line)) {
    if (line.empty()) continue;

    Packet p;

    if (line.find("/nick ") == 0) {
      std::string new_name = line.substr(6);
      p << static_cast<std::uint8_t>(CommandID::NICKNAME) << new_name;
    }
    else if (line.find("/join ") == 0) {
      if (!m_current_channel.empty()) {
        Console::print(
            ConsoleLevel::System,
            "You are already connected to channel #" + m_current_channel +
                ". Type /leave before trying to join other channel.");
        Console::print(ConsoleLevel::Prompt, "> ");
        continue;
      }
      std::string channel_name = line.substr(6);
      p << static_cast<std::uint8_t>(CommandID::JOIN) << channel_name;
    }
    else if (line.find("/channels") == 0) {
      p << static_cast<std::uint8_t>(CommandID::LIST_CHANNELS);
    }
    else if (line.find("/users") == 0) {
      p << static_cast<std::uint8_t>(CommandID::LIST_USERS);
    }
    else if (line.find("/msg ") == 0) {
      std::string remaining = line.substr(5);
      size_t space_pos = remaining.find(' ');

      if (space_pos != std::string::npos &&
          space_pos < remaining.length() - 1) {
        std::string target = remaining.substr(0, space_pos);
        std::string text = remaining.substr(space_pos + 1);

        p << static_cast<std::uint8_t>(CommandID::PRIVATE_MSG) << target
          << text;
      }
      else {
        Console::print(ConsoleLevel::System,
                       "Usage: /msg <nickname> <message>.");
      }
    }
    else if (line.find("/leave") == 0) {
      if (m_current_channel.empty()) {
        Console::print(ConsoleLevel::System,
                       "You are not part of any channel right now.");
        Console::print(ConsoleLevel::Prompt, "> ");
        continue;
      }

      Console::print(ConsoleLevel::System,
                     "You are leaving #" + m_current_channel + " channel.");
      p << static_cast<std::uint8_t>(CommandID::LEAVE) << m_current_channel;
      m_current_channel = "";
    }
    else if (line.find("/create ") == 0) {
      std::string channel_name = line.substr(8);
      p << static_cast<std::uint8_t>(CommandID::CREATE) << channel_name;
    }
    else {
      if (m_current_channel.empty()) {
        Console::print(ConsoleLevel::System,
                       "Join a channel first via /join <server_name>.");

        Console::print(ConsoleLevel::Prompt, "> ");
        continue;
      }
      p << static_cast<std::uint8_t>(CommandID::MSG) << m_current_channel
        << line;
    }

    m_socket.send(p);

    Console::print(ConsoleLevel::Prompt, "> ");
  }
}

void Client::setup_message_handler(
    std::function<void(std::shared_ptr<Message>)> handler) {
  m_message_handler = handler;
}

void Client::send_message(const std::string& line) {
  if (!m_connected) {
    Console::print(ConsoleLevel::Error, "Not connected to server.");
    return;
  }
  if (line.empty()) {
    return;
  }

  Packet p;

  if (line.find("/nick ") == 0) {
    std::string new_name = line.substr(6);
    p << static_cast<std::uint8_t>(CommandID::NICKNAME) << new_name;
  }
  else if (line.find("/join ") == 0) {
    std::string channel_name = line.substr(6);
    p << static_cast<std::uint8_t>(CommandID::JOIN) << channel_name;
  }
  else if (line.find("/create ") == 0) {
    std::string channel_name = line.substr(8);
    p << static_cast<std::uint8_t>(CommandID::CREATE) << channel_name;
  }
  else if (line.find("/channels") == 0) {
    p << static_cast<std::uint8_t>(CommandID::LIST_CHANNELS);
  }
  else if (line.find("/users") == 0) {
    p << static_cast<std::uint8_t>(CommandID::LIST_USERS);
  }
  else if (line.find("/leave") == 0) {
    p << static_cast<std::uint8_t>(CommandID::LEAVE) << m_current_channel;
    m_current_channel = "";
  }
  else {
    if (!m_current_channel.empty()) {
      p << static_cast<std::uint8_t>(CommandID::MSG) << m_current_channel
        << line;
    }
  }

  if (p.get_size() > 0) {
    m_socket.send(p);
  }
}
