#pragma once

#include <cstdint>

enum class CommandID : std::uint8_t {
  NONE = 0,
  REGISTER = 1,
  LOGIN = 2,
  JOIN = 3,
  MSG = 4,
  LEAVE = 5,
  NICKNAME = 6,
  LIST_CHANNELS = 7,
  LIST_USERS = 8,
  CREATE = 9,
  PRIVATE_MSG = 10,
  SET_SELF_NAME = 11,
  DM_HISTORY = 12,
  ERROR = 13
};

enum class UpdateType : std::uint8_t {
  ManualRequest = 0,
  BackgroundPush = 1,
};
