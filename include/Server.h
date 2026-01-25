#include <string>
#include <unordered_map>

#include "Channel.h"

class Server {
  std::unordered_map<int, User> m_users{};
  std::unordered_map<std::string, Channel> m_channels{};
};
