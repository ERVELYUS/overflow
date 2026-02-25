#include "Client.h"

#include <cstdint>
#include <iostream>
#include <ostream>
#include <stdexcept>

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
  m_running = true;

  m_recieve_thread = std::thread([this]() {
    while (m_running) {
      Packet packet{};

      if (m_socket.recv(packet)) {
        handle_server_message(packet);
      }
      else {
        if (m_running) {
          std::cout << "\n[System] Disconnected from server.\n";
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

    std::cout << "\r[" << sender << "]: " << content << "\n" << std::flush;

    std::cout << "> " << std::flush;
  }
  else if (id == CommandID::LIST_CHANNELS) {
    std::cout << "[System] List of all available channels:\n" << std::flush;

    std::uint32_t channels_count{};
    packet >> channels_count;

    std::string channel_name;

    ChannelsList channelNames;

    for (std::uint32_t i = 0; i < channels_count; ++i) {
      packet >> channel_name;
      channelNames << channel_name;
      std::cout << '#' << channel_name << '\n';
    }

    auto message = std::make_shared<ChannelsList>(channelNames);
    if (m_message_handler) m_message_handler(message);

    std::cout << "> " << std::flush;
  }
  else if (id == CommandID::LIST_USERS) {
    std::cout << "[System] List of all users:\n" << std::flush;

    std::uint32_t users_count{};
    packet >> users_count;

    std::string user_name;

    UsersList userNames;

    for (std::uint32_t i = 0; i < users_count; ++i) {
      packet >> user_name;
      userNames << user_name;
      std::cout << '@' << user_name << '\n';
    }

    auto message = std::make_shared<UsersList>(userNames);
    if (m_message_handler) m_message_handler(message);

    std::cout << "> " << std::flush;
  }
  else if (id == CommandID::NICKNAME) {
    bool successful{};
    packet >> successful;
    if (successful) {
      std::cout << "[System] Name changed successfully.\n" << std::flush;
    }
    else {
      std::cout
          << "[System] Invalid nickname. Use 3-20 alphanumeric characters.\n"
          << std::flush;
    }

    std::cout << "> " << std::flush;
  }
  else if (id == CommandID::JOIN) {
    bool successful{};
    packet >> successful;
    if (successful) {
      std::string channel_name;
      packet >> channel_name;
      m_current_channel = channel_name;
      std::cout << "[System] Connected to channel #" << channel_name << ".\n"
                << std::flush;
    }
    else {
      std::cout << "Channel does not exist. Use /create to create it.\n"
                << std::flush;
    }

    std::cout << "> " << std::flush;
  }
  else if (id == CommandID::PRIVATE_MSG) {
    std::string sender, content;
    packet >> sender >> content;
    std::cout << "\r[@" << sender << "]: " << content << "\n> " << std::flush;
  }
  else if (id == CommandID::CREATE) {
    std::string message{};
    packet >> message;
    std::cout << message << std::flush;
    std::cout << "> " << std::flush;
  }
  else if (id == CommandID::ERROR) {
    std::string error_msg;
    packet >> error_msg;
    std::cout << "\r[Error] " << error_msg << "\n> " << std::flush;
  }
}

void Client::run() {
  std::string line{};
  std::cout << "> " << std::flush;

  while (m_running && std::getline(std::cin, line)) {
    if (line.empty()) continue;

    Packet p;

    if (line.find("/nick ") == 0) {
      std::string new_name = line.substr(6);
      p << static_cast<std::uint8_t>(CommandID::NICKNAME) << new_name;
      m_nickname = new_name;
    }
    else if (line.find("/join ") == 0) {
      if (!m_current_channel.empty()) {
        std::cout << "[System] You are already connected to channel #"
                  << m_current_channel
                  << ". Type /leave before trying to join other channel.\n"
                  << std::flush;
        std::cout << "> " << std::flush;
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
        std::cout << "[System] Usage: /msg <nickname> <message>\n> "
                  << std::flush;
      }
    }
    else if (line.find("/leave") == 0) {
      if (m_current_channel.empty()) {
        std::cout << "[System] You are not a part of any channel right now.\n"
                  << std::flush;
        std::cout << "> " << std::flush;
        continue;
      }
      std::cout << "[System] You are leaving #" << m_current_channel
                << " channel.\n"
                << std::flush;
      p << static_cast<std::uint8_t>(CommandID::LEAVE) << m_current_channel;
      m_current_channel = "";
    }
    else if (line.find("/create ") == 0) {
      std::string channel_name = line.substr(8);
      p << static_cast<std::uint8_t>(CommandID::CREATE) << channel_name;
    }
    else {
      if (m_current_channel.empty()) {
        std::cout << "[System] Join a channel first via /join <server_name>\n"
                  << std::flush;
        std::cout << "> " << std::flush;
        continue;
      }
      p << static_cast<std::uint8_t>(CommandID::MSG) << m_current_channel
        << line;
    }

    m_socket.send(p);

    std::cout << "> " << std::flush;
  }
}

void Client::setup_message_handler(
    std::function<void(std::shared_ptr<Message>)> handler) {
  m_message_handler = handler;
}
