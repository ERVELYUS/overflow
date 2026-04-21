#pragma once
#include <string>

enum class ConsoleLevel {
  Info,
  System,
  Error,
  Prompt,
};

class Console {
 public:
  Console() = delete;

  static void set_enabled(bool e);
  static bool enabled();

  static void print(ConsoleLevel level, const std::string& msg);
};
