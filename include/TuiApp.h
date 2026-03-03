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
  std::vector<std::string> m_users_list{};

  std::mutex m_chat_mutex{};
  std::thread m_network_thread{};
  bool m_running = false;

  ftxui::ScreenInteractive m_screen;
  std::string m_input_buffer{};
  ftxui::Component m_input_field;

  int m_tab_selected = 0;
  std::vector<std::string> m_tab_entries = {"DM", "Groups"};
  ftxui::Component m_tab_toggle;
  ftxui::Component m_settings_button;
  ftxui::Component m_main_container;
};
