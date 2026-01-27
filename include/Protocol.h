#pragma once

#include <cstdint>

enum class CommandID : std::uint8_t {
  None = 0,
  Join = 1,
  Msg = 2,
  Leave = 3,
  Nickname = 4,
  Error = 5,
};
