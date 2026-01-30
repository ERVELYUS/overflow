#include "User.h"

User::User(TcpSocket socket, std::string_view name)
    : m_socket(std::move(socket)), m_name(name) {}

void User::set_name(std::string_view new_name) { m_name = new_name; }

std::string_view User::get_name() { return m_name; }

TcpSocket& User::get_socket() { return m_socket; }

void User::send(const Packet& packet) { m_socket.send(packet); }

bool User::recv(Packet& packet) {
  if (!m_socket.recv(packet)) {
    // Need to handle user disconnection
    return false;
  }
  return true;
}
