#pragma once
#include <ftxui/component/screen_interactive.hpp>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "Client.h"
#include "Message.h"

class TuiApp {
 public:
  TuiApp();
  ~TuiApp();
  void run();

 private:
  // Callback invoked by Client on network thread
  void on_message_received(std::shared_ptr<Message> msg);

  Client m_client{};
  std::vector<std::string> m_chat_history{};
  std::vector<std::string> m_users_list{};

  std::mutex m_chat_mutex{};
  std::thread m_network_thread{};
  std::thread m_poll_thread{};
  bool m_running = false;

  // FTXUI
  ftxui::ScreenInteractive m_screen;
  std::string m_input_buffer{};
  ftxui::Component m_input_field;

  // Header controls
  int m_tab_selected = 0;
  std::vector<std::string> m_tab_entries = {"1. DM", "2. Groups",
                                            "0. Settings"};
  ftxui::Component m_tab_toggle;
  ftxui::Component m_main_container;
};
