#pragma once

#include <cstdint>

enum class CommandID : std::uint8_t {
  NONE = 0,
  JOIN = 1,
  MSG = 2,
  LEAVE = 3,
  NICKNAME = 4,
  LIST = 5,
  ERROR = 6
};
