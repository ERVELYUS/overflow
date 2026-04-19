#include "Client.h"

#include <cstdint>
#include <stdexcept>

#include "Console.h"
#include "Message.h"
#include "Protocol.h"
#include "cppcon/AddrInfoResolver.h"

Client::Client() : m_running(false) {}

Client::~Client() {
  m_running = false;
  m_socket.close();
  if (m_receive_thread.joinable()) {
    m_receive_thread.join();
  }
}

void Client::connect(const std::string& ip, const std::string& port) {
  auto tcp_endpoints = AddrInfoResolver::resolve(ip, port);
  if (tcp_endpoints.empty()) {
    throw std::runtime_error("Could not resolve server address");
  }

  m_socket.connect(tcp_endpoints[0]);
  m_connected = true;
  m_running = true;

  m_receive_thread = std::thread([this]() {
    while (m_running) {
      Packet packet{};

      if (m_socket.recv(packet)) {
        handle_server_message(packet);
      }
      else {
        if (m_running) {
          // Console::print(ConsoleLevel::System, "Disconnected from server.");
          m_running = false;
        }
        break;
      }
    }
  });
}

void Client::handle_server_message(Packet& packet) {
  std::uint8_t id_raw{};
  packet >> id_raw;
  CommandID id = static_cast<CommandID>(id_raw);

  auto notify_ui = [this](ConsoleLevel level, const std::string& text) {
    if (m_message_handler) {
      m_message_handler(std::make_shared<SystemMessage>(level, text));
    }
  };

  switch (id) {
    case CommandID::MSG: {
      std::string sender, content;
      packet >> sender >> content;
      if (m_message_handler) {
        m_message_handler(std::make_shared<UserMessage>(sender, content));
      }
      break;
    }
    case CommandID::LOGIN:
    case CommandID::REGISTER: {
      bool successful{};
      std::string message;
      packet >> successful >> message;

      if (successful) {
        m_authenticated = true;
        m_nickname = message;

        if (m_message_handler) {
          m_message_handler(std::make_shared<SelfNameMessage>(m_nickname));
        }

        if (id == CommandID::LOGIN) {
          notify_ui(ConsoleLevel::System,
                    "Login successful! Welcome, " + m_nickname);
        }
        else {
          notify_ui(ConsoleLevel::System,
                    "Registration successful! Welcome, " + m_nickname);
        }
      }
      else {
        notify_ui(ConsoleLevel::Error, message);
      }
      break;
    }
    case CommandID::NICKNAME:
    case CommandID::SET_SELF_NAME: {
      bool successful{};
      packet >> successful;
      if (successful) {
        packet >> m_nickname;
        if (m_message_handler) {
          m_message_handler(std::make_shared<SelfNameMessage>(m_nickname));
        }
      }
      else {
        notify_ui(ConsoleLevel::Error, "Nickname change failed.");
      }
      break;
    }

    case CommandID::LIST_CHANNELS: {
      std::uint8_t type_raw{};
      std::uint32_t count{};
      packet >> type_raw >> count;

      auto channels = std::make_shared<ChannelsList>();
      for (std::uint32_t i = 0; i < count; ++i) {
        std::string name;
        packet >> name;
        *channels << name;
      }

      if (m_message_handler) m_message_handler(channels);
      break;
    }

    case CommandID::LIST_USERS: {
      std::uint8_t type_raw{};
      std::uint32_t count{};
      packet >> type_raw >> count;

      auto users = std::make_shared<UsersList>();
      for (std::uint32_t i = 0; i < count; ++i) {
        std::string name;
        packet >> name;
        *users << name;
      }

      if (m_message_handler) m_message_handler(users);
      break;
    }
    case CommandID::JOIN: {
      bool successful{};
      packet >> successful;
      if (successful) {
        packet >> m_current_channel;

        if (m_message_handler) {
          m_message_handler(
              std::make_shared<JoinedChannelMessage>(m_current_channel));
        }

        notify_ui(ConsoleLevel::System, "Joined #" + m_current_channel);
      }
      else {
        notify_ui(ConsoleLevel::Error, "Failed to join channel.");
      }
      break;
    }
    case CommandID::PRIVATE_MSG: {
      std::string sender, content;
      packet >> sender >> content;
      if (m_message_handler) {
        m_message_handler(std::make_shared<UserMessage>(sender, content));
      }
      break;
    }

    case CommandID::CREATE: {
      std::string message;
      packet >> message;
      notify_ui(ConsoleLevel::System, message);
      break;
    }

    case CommandID::ERROR: {
      std::string error_msg;
      packet >> error_msg;
      notify_ui(ConsoleLevel::Error, "Server Error: " + error_msg);
      break;
    }

    default:
      notify_ui(ConsoleLevel::Error,
                "Received unknown command ID from server.");
      break;
  }
}

void Client::register_message_callback(
    std::function<void(std::shared_ptr<Message>)> handler) {
  m_message_handler = handler;
}

void Client::send_message(const std::string& line) {
  if (!m_connected) {
    Console::print(ConsoleLevel::Error, "Not connected to server.");
    return;
  }

  auto p = build_command_packet(line);
  if (p.has_value()) {
    m_socket.send(p.value());
  }
}

std::optional<Packet> Client::build_command_packet(const std::string& line) {
  if (line.empty()) {
    return std::nullopt;
  }

  // Enforce authentication
  if (!is_authenticated()) {
    const bool is_register = line.rfind("/register ", 0) == 0;
    const bool is_login = line.rfind("/login ", 0) == 0;

    if (!is_register && !is_login) {
      Console::print(ConsoleLevel::Error, "Please /register or /login first.");
      return std::nullopt;
    }
  }

  Packet p;
  if (line.find("/nick ") == 0) {
    std::string new_name = line.substr(6);
    p << static_cast<std::uint8_t>(CommandID::NICKNAME) << new_name;
  }
  else if (line.find("/register ") == 0) {
    std::string data_line = line.substr(10);
    size_t space_pos = data_line.find(' ');

    if (space_pos != std::string::npos && space_pos < data_line.length() - 1) {
      std::string username = data_line.substr(0, space_pos);
      std::string password = data_line.substr(space_pos + 1);
      p << static_cast<std::uint8_t>(CommandID::REGISTER) << username
        << password;
    }
    else {
      Console::print(ConsoleLevel::System,
                     "Usage: /register <nickname> <password>.");
      Console::print(ConsoleLevel::Prompt, "> ");
      return std::nullopt;
    }
  }
  else if (line.find("/login ") == 0) {
    std::string data_line = line.substr(7);
    size_t space_pos = data_line.find(' ');

    if (space_pos != std::string::npos && space_pos < data_line.length() - 1) {
      std::string username = data_line.substr(0, space_pos);
      std::string password = data_line.substr(space_pos + 1);
      p << static_cast<std::uint8_t>(CommandID::LOGIN) << username << password;
    }
    else {
      Console::print(ConsoleLevel::System,
                     "Usage: /login <nickname> <password>.");
      Console::print(ConsoleLevel::Prompt, "> ");
      return std::nullopt;
    }
  }
  else if (line.find("/join ") == 0) {
    if (!m_current_channel.empty()) {
      Console::print(ConsoleLevel::System,
                     "You are already connected to channel #" +
                         m_current_channel +
                         ". Type /leave before trying to join other channel.");
      Console::print(ConsoleLevel::Prompt, "> ");
      return std::nullopt;
    }
    std::string channel_name = line.substr(6);
    p << static_cast<std::uint8_t>(CommandID::JOIN) << channel_name;
  }
  else if (line.find("/create ") == 0) {
    std::string channel_name = line.substr(8);
    p << static_cast<std::uint8_t>(CommandID::CREATE) << channel_name;
  }
  else if (line.find("/channels") == 0) {
    p << static_cast<std::uint8_t>(CommandID::LIST_CHANNELS);
  }
  else if (line.find("/users") == 0) {
    p << static_cast<std::uint8_t>(CommandID::LIST_USERS);
  }
  else if (line.find("/leave") == 0) {
    if (m_current_channel.empty()) {
      Console::print(ConsoleLevel::System,
                     "You are not part of any channel right now.");
      Console::print(ConsoleLevel::Prompt, "> ");
      return std::nullopt;
    }

    Console::print(ConsoleLevel::System,
                   "You are leaving #" + m_current_channel + " channel.");
    p << static_cast<std::uint8_t>(CommandID::LEAVE) << m_current_channel;
    m_current_channel = "";
  }
  else if (line.find("/pm ") == 0) {
    std::string data_line = line.substr(4);
    size_t space_pos = data_line.find(' ');

    if (space_pos != std::string::npos && space_pos < data_line.length() - 1) {
      std::string target_name = data_line.substr(0, space_pos);
      std::string message_text = data_line.substr(space_pos + 1);
      p << static_cast<std::uint8_t>(CommandID::PRIVATE_MSG) << target_name
        << message_text;
    }
    else {
      Console::print(ConsoleLevel::System, "Usage: /pm <nickname> <message>.");
      Console::print(ConsoleLevel::Prompt, "> ");
      return std::nullopt;
    }
  }
  else if (line[0] != '/') {
    if (m_current_channel.empty()) return std::nullopt;
    p << static_cast<std::uint8_t>(CommandID::MSG) << m_current_channel << line;
  }

  return p;
}
