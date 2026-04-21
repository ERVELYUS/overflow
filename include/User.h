#pragma once

#include <cppcon/Packet.h>
#include <cppcon/TcpSocket.h>

#include <string>

class User {
  TcpSocket m_socket{};
  std::string m_name{};
  int m_db_id{-1};
  bool m_authenticated = false;

 public:
  User(TcpSocket socket, std::string_view name);

  void set_id(const int id) { m_db_id = id; }
  [[nodiscard]] int get_id() const { return m_db_id; }

  void set_name(std::string_view new_name);

  [[nodiscard]] std::string_view get_name() const;
  TcpSocket& get_socket();

  void send(const Packet& packet);
  bool recv(Packet& packet);

  void authenticate() { m_authenticated = true; }
  [[nodiscard]] bool is_authenticated() const { return m_authenticated; }
};
