#pragma once

#include <vector>

#include "cppcon/UniversalTypes.h"

class Channel {
  int m_db_id{-1};
  std::vector<socket_t> m_user_fds{};

 public:
  void set_id(const int id) { m_db_id = id; }
  [[nodiscard]] int get_id() const { return m_db_id; }

  void add_user(socket_t user);
  void remove_user(socket_t user);
  [[nodiscard]] const std::vector<socket_t>& get_users() const {
    return m_user_fds;
  }
};