#pragma once

#include <cppcon/Packet.h>
#include <cppcon/TcpSocket.h>
#include <Message.h>

#include <atomic>
#include <string>
#include <thread>
#include <functional>

class Client {
  TcpSocket m_socket{};
  std::atomic<bool> m_running{};
  std::thread m_recieve_thread{};
  std::function<void(std::shared_ptr<Message>)> m_messageHandler;

  std::string m_nickname{};
  std::string m_current_channel{};

  void handle_server_message(Packet& packet);

 public:
  Client();
  ~Client();

  void connect(const std::string& ip, const std::string& port);

  void run();

  void SetupMessageHandler(std::function<void(std::shared_ptr<Message>)>);
};
