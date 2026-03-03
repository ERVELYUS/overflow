#include "TuiApp.h"

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <memory>

#include "tui/Layouts.h"

using namespace ftxui;

TuiApp::TuiApp()
    : m_running(true), m_screen(ScreenInteractive::TerminalOutput()) {
  m_tab_toggle = Toggle(&m_tab_entries, &m_tab_selected);

  m_settings_button = Button("Settings", [this] {
    // TODO: Add settings
  });
  m_input_field = Input(&m_input_buffer, "type here...");
  m_input_field = CatchEvent(m_input_field, [this](Event event) {
    if (event == Event::Return) {
      if (!m_input_buffer.empty()) {
        m_client.send_message(m_input_buffer);

        m_input_buffer = "";

        return true;
      }
    }
    return false;
  });

  m_main_container = Container::Vertical({
      Container::Horizontal({
          m_tab_toggle,
          m_settings_button,
      }),
      m_input_field,
  });

  m_client.setup_message_handler(
      [this](auto msg) { this->on_message_received(msg); });
};

TuiApp::~TuiApp() {
  m_running = false;
  if (m_network_thread.joinable()) {
    m_network_thread.join();
  }
}

void TuiApp::on_message_received(std::shared_ptr<Message> msg) {
  // Lock because of multiple threads trying to access the same data
  std::lock_guard<std::mutex> lock(m_chat_mutex);
  if (auto user_msg = std::dynamic_pointer_cast<UserMessage>(msg)) {
    m_chat_history.push_back(user_msg->m_name + ": " + user_msg->m_msg);
  }
  else if (auto list_msg = std::dynamic_pointer_cast<UsersList>(msg)) {
    m_users_list.clear();

    for (const auto& name : list_msg->m_users) {
      m_users_list.push_back(name);
    }
  }
  m_screen.PostEvent(Event::Custom);
}

void TuiApp::run() {
  m_network_thread = std::thread([this]() {
    try {
      m_client.connect("0.0.0.0", "8080");  // TODO: REMOVE HARDCODED
    }
    catch (...) { /* TODO: HANDLE ERRORS*/
    }
  });

  auto main_conatainer = ftxui::Container::Vertical({
      m_input_field,
  });

  auto renderer = Renderer(m_main_container, [this] {
    std::lock_guard<std::mutex> lock(m_chat_mutex);
    return TuiDesign::RenderMainLayout(m_tab_toggle->Render(),
                                       m_settings_button->Render(),
                                       m_chat_history, m_input_field->Render());
  });

  m_screen.Loop(renderer);
}
