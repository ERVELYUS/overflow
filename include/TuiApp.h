#pragma once
#include <ftxui/component/screen_interactive.hpp>
#include <mutex>

#include "Client.h"

class TuiApp {
 public:
  TuiApp();
  ~TuiApp();
  void run();

 private:
  // Callback
  void on_message_received(std::shared_ptr<Message> msg);

  Client m_client{};
  std::vector<std::string> m_chat_history{};

  std::mutex m_chat_mutex{};
  std::thread m_network_thread{};
  bool m_running = false;

  ftxui::ScreenInteractive m_screen;
};
