#include "TuiApp.h"

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <memory>

using namespace ftxui;

TuiApp::TuiApp()
    : m_running(true), m_screen(ScreenInteractive::TerminalOutput()) {
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
  if (auto user_msg = std::dynamic_pointer_cast<UserMessage>(msg)) {
    // Lock because of multiple threads trying to access the same data
    std::lock_guard<std::mutex> lock(m_chat_mutex);
    m_chat_history.push_back(user_msg->m_name + ": " + user_msg->m_msg);
    m_screen.PostEvent(Event::Custom);
  }
}

void TuiApp::run() {
  m_network_thread = std::thread([this]() {
    try {
      m_client.connect("127.0.0.1", "8080");
      m_client.run();
    }
    catch (...) { /* TODO: HANDLE ERRORS*/
    }
  });

  auto renderer = Renderer([this] {
    // Lock while reading!
    std::lock_guard<std::mutex> lock(m_chat_mutex);
    Elements lines;
    for (auto& line : m_chat_history) {
      lines.push_back(text(line));
    }

    return window(text(" Overflow TUI "), vbox(std::move(lines)) | frame);
  });

  m_screen.Loop(renderer);
}
