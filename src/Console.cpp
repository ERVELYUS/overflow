#include "Console.h"

#include <iostream>
#include <mutex>

static std::mutex g_console_mutex;
static bool g_console_enabled = true;

void Console::set_enabled(bool e) {
  std::lock_guard<std::mutex> lock(g_console_mutex);
  g_console_enabled = e;
}

bool Console::enabled() {
  std::lock_guard<std::mutex> lock(g_console_mutex);
  return g_console_enabled;
}

void Console::print(ConsoleLevel level, const std::string& msg) {
  std::lock_guard<std::mutex> lock(g_console_mutex);
  if (!g_console_enabled) {
    return;
  }

  switch (level) {
    case ConsoleLevel::Info:
      std::cout << msg << std::endl;
      break;
    case ConsoleLevel::System:
      std::cout << "[System] " << msg << std::endl;
      break;
    case ConsoleLevel::Error:
      std::cerr << "[Error] " << msg << std::endl;
      break;
    case ConsoleLevel::Prompt:
      std::cout << msg << std::flush;
      break;
  }
}
