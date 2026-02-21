#include "Channel.h"

#include <algorithm>

#include "cppcon/UniversalTypes.h"

void Channel::add_user(socket_t user) { m_user_fds.push_back(user); }

void Channel::remove_user(socket_t fd) {
  m_user_fds.erase(std::remove(m_user_fds.begin(), m_user_fds.end(), fd),
                   m_user_fds.end());
}
