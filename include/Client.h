#pragma once

#include <Message.h>
#include <cppcon/Packet.h>
#include <cppcon/TcpSocket.h>

#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <thread>

class Client {
  TcpSocket m_socket{};
  std::atomic<bool> m_running{};
  std::atomic<bool> m_connected{false};
  std::atomic<bool> m_authenticated{false};
  std::thread m_receive_thread{};
  std::function<void(std::shared_ptr<Message>)> m_message_handler;

  std::string m_nickname{};
  std::string m_current_channel{};

  void handle_server_message(Packet& packet);
  std::optional<Packet> build_command_packet(const std::string& line);

 public:
  Client();
  ~Client();

  void register_message_callback(std::function<void(std::shared_ptr<Message>)>);

  [[nodiscard]] bool is_running() const { return m_running; }
  [[nodiscard]] bool is_authenticated() const { return m_authenticated; }

  void connect(const std::string& ip, const std::string& port);
  void send_message(const std::string& line);
};
