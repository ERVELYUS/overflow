#pragma once

#include <cppcon/Packet.h>
#include <cppcon/TcpSocket.h>

#include <atomic>
#include <string>
#include <thread>

class Client {
  TcpSocket m_socket{};
  std::atomic<bool> m_running{};
  std::thread m_recieve_thread{};

  std::string m_nickname{};
  std::string m_current_channel{};

  void handle_server_message(Packet& packet);

 public:
  Client();
  ~Client();

  void connect(const std::string& ip, const std::string& port);

  void run();
};
