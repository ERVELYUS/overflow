#include "Channel.h"

#include <algorithm>

void Channel::add_user(User* user) { m_users.push_back(user); }

void Channel::remove_user(User* user) {
  m_users.erase(std::remove(m_users.begin(), m_users.end(), user),
                m_users.end());
}

void Channel::broadcast(const Packet& packet, User* sender) {
  for (auto member : m_users) {
    if (member == sender) {
      continue;
    }
    member->send(packet);
  }
}
