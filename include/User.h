#pragma once

#include <cppcon/Packet.h>
#include <cppcon/TcpSocket.h>

#include <string>

class User {
  TcpSocket m_socket{};
  std::string m_name{};
  bool m_authenticated = false;

 public:
  User(TcpSocket socket, std::string_view name);

  void set_name(std::string_view new_name);

  std::string_view get_name() const;
  TcpSocket& get_socket();

  void send(const Packet& packet);
  bool recv(Packet& packet);

  void authenticate();
};
