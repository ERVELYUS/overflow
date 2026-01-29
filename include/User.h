#pragma once

#include <cppcon/Packet.h>
#include <cppcon/TcpSocket.h>

#include <string>

class User {
  TcpSocket m_socket{};
  std::string m_name{};

 public:
  User(TcpSocket socket, std::string_view name);

  std::string_view get_name();
  TcpSocket& get_socket();

  void send(const Packet& packet);
  bool recv(Packet& packet);
};
