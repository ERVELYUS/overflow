#include <cppcon/Packet.h>

#include <vector>

#include "User.h"

class Channel {
  std::vector<User*> m_users{};

 public:
  void add_user(User* user);
  void remove_user(User* user);

  void broadcast(const Packet& packet, User* sender);
};
