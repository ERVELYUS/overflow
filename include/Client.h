#pragma once

#include <Message.h>
#include <cppcon/Packet.h>
#include <cppcon/TcpSocket.h>

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>

class Client {
  TcpSocket m_socket{};
  std::atomic<bool> m_running{};
  std::atomic<bool> m_connected{false};
  std::thread m_recieve_thread{};
  std::function<void(std::shared_ptr<Message>)> m_message_handler;

  std::string m_nickname{};
  std::string m_current_channel{};

  void handle_server_message(Packet& packet);

 public:
  Client();
  ~Client();

  void connect(const std::string& ip, const std::string& port);

  void run();

  void setup_message_handler(std::function<void(std::shared_ptr<Message>)>);

  void send_message(const std::string& line);
};
