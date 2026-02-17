#include "Client.h"

#include <cstdint>
#include <iostream>
#include <ostream>
#include <stdexcept>

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
        this->handle_server_message(packet);
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

    std::cout << "\r[" << sender << "]: " << content << "\n" << std::flush;

    std::cout << "> " << std::flush;
  }
  else if (id == CommandID::LIST) {
    std::cout << "[System] List of all available channels:\n" << std::flush;

    std::uint32_t channels_count{};
    packet >> channels_count;

    std::string channel_name;
    for (std::uint32_t i = 0; i < channels_count; ++i) {
      packet >> channel_name;
      std::cout << '#' << channel_name << '\n';
    }
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
  else if (id == CommandID::CREATE) {
    std::string message{};
    packet >> message;
    std::cout << message << std::flush;
    std::cout << "> " << std::flush;
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
    else if (line.find("/list") == 0) {
      p << static_cast<std::uint8_t>(CommandID::LIST);
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
