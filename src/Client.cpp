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
  auto endpoints = AddrInfoResolver::resolve(ip, port);
  if (endpoints.empty()) {
    throw std::runtime_error("Could not resolve server address!");
  }

  m_socket.connect(endpoints[0]);
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
      // TODO: Add name filters
      m_nickname = new_name;
      std::cout << "[System] Name changed to " << new_name << ".\n"
                << std::flush;
    }
    else if (line.find("/join ") == 0) {
      std::string channel_name = line.substr(6);
      p << static_cast<std::uint8_t>(CommandID::JOIN) << channel_name;
      m_current_channel = channel_name;
      std::cout << "[System] Connected to channel #" << channel_name << ".\n"
                << std::flush;
    }
    else {
      if (m_current_channel.empty()) {
        std::cout << "[System] Join a channel first via /join server_name\n"
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
