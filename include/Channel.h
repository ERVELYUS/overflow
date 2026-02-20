#pragma once

#include <cppcon/Packet.h>

#include <vector>

#include "cppcon/UniversalTypes.h"

class Channel {
  std::vector<socket_t> m_user_fds{};

 public:
  void add_user(socket_t user);
  void remove_user(socket_t user);
  const std::vector<socket_t>& get_users() const { return m_user_fds; }
};
