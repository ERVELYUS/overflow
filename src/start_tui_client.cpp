#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <memory>
#include <mutex>

#include "Client.h"

using namespace ftxui;

int main() {
  std::vector<std::string> chat_history{};
  std::mutex chat_mutex;

  auto screen = ScreenInteractive::TerminalOutput();
  Client client{};

  client.setup_message_handler([&](std::shared_ptr<Message> msg) {
    if (auto user_msg = std::dynamic_pointer_cast<UserMessage>(msg)) {
      std::lock_guard<std::mutex> lock(chat_mutex);
      chat_history.push_back(user_msg->m_name + ": " + user_msg->m_msg);

      screen.PostEvent(Event::Custom);
    }
  });

  std::thread client_thread([&]() {
    try {
      client.connect("0.0.0.0", "8080");
    }
    catch (const std::exception& e) {
    }
  });

  auto renderer = Renderer([&] {
    std::lock_guard<std::mutex> lock(chat_mutex);
    Elements lines;
    for (const auto& line : chat_history) {
      lines.push_back(text(line));
    }

    return window(text(" Overflow Chat Test "), vbox(std::move(lines)));
  });

  screen.Loop(renderer);

  if (client_thread.joinable()) {
    client_thread.join();
  }
}
