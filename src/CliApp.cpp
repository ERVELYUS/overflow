#include "CliApp.h"

#include <iostream>

#include "Console.h"
#include "Message.h"

CliApp::CliApp() { setup_callbacks(); }

void CliApp::setup_callbacks() {
  m_client.register_message_callback([](std::shared_ptr<Message> msg) {
    auto print_prompt = [] {
      Console::print(ConsoleLevel::Prompt, "> ");
      std::cout.flush();
    };

    if (auto user_msg = std::dynamic_pointer_cast<UserMessage>(msg)) {
      Console::print(ConsoleLevel::Info,
                     "[" + user_msg->m_name + "] " + user_msg->m_msg);
    }
    else if (auto sys_msg = std::dynamic_pointer_cast<SystemMessage>(msg)) {
      Console::print(sys_msg->m_level, sys_msg->m_text);
    }
    else if (auto self_msg = std::dynamic_pointer_cast<SelfNameMessage>(msg)) {
      Console::print(ConsoleLevel::System,
                     "Your name is now: " + self_msg->m_name);
    }
    else if (auto users_msg = std::dynamic_pointer_cast<UsersList>(msg)) {
      if (users_msg->m_update_type == UpdateType::BackgroundPush) {
        return;
      }

      Console::print(ConsoleLevel::System, "Active users:");

      if (users_msg->m_users.empty()) {
        Console::print(ConsoleLevel::Info, "  (none)");
      }
      else {
        for (const auto& name : users_msg->m_users) {
          Console::print(ConsoleLevel::Info, "  @" + name);
        }
      }
    }
    else if (auto channels_msg = std::dynamic_pointer_cast<ChannelsList>(msg)) {
      if (channels_msg->m_update_type == UpdateType::BackgroundPush) {
        return;
      }

      Console::print(ConsoleLevel::System, "Available channels:");

      if (channels_msg->m_channels.empty()) {
        Console::print(ConsoleLevel::Info, "  (none)");
      }
      else {
        for (const auto& name : channels_msg->m_channels) {
          Console::print(ConsoleLevel::Info, "  #" + name);
        }
      }
    }

    print_prompt();
  });
}

void CliApp::run(const std::string& ip, const std::string& port) {
  try {
    std::cout << "Connecting to " << ip << ':' << port << "...\n";
    m_client.connect(ip, port);
    std::cout << "Connected. Use /register <nickname> <password> or "
                 "/login <nickname> <password>.\n";
    std::string line{};

    // We check m_client.is_running() so the loop breaks if the server drops us.
    while (m_client.is_running() && std::getline(std::cin, line)) {
      m_client.send_message(line);
    }

    std::cout << "Disconnected from server.\n";
  }
  catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << '\n';
  }
}
