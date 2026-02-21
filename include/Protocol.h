#pragma once

#include <cstdint>

enum class CommandID : std::uint8_t {
  NONE = 0,
  JOIN = 1,
  MSG = 2,
  LEAVE = 3,
  NICKNAME = 4,
  LIST_CHANNELS = 5,
  LIST_USERS = 6,
  CREATE = 7,
  PRIVATE_MSG = 8,
  ERROR = 9
};
