#pragma once

#include <list>
#include <string>
#include <utility>

#include "Console.h"
#include "Protocol.h"

struct Message {
  virtual ~Message() = default;  // RTTI
};

struct SystemMessage : Message {
  ConsoleLevel m_level;
  std::string m_text;
  SystemMessage(const ConsoleLevel l, std::string t)
      : m_level(l), m_text(std::move(t)) {}
};

struct UserMessage : Message {
  UserMessage(std::string name, std::string msg)
      : m_name(std::move(name)), m_msg(std::move(msg)) {}
  std::string m_name;
  std::string m_msg;
};

struct DMHistoryLine {
  std::string m_sender;
  std::string m_text;
};

struct DMHistoryMessage : Message {
  explicit DMHistoryMessage(std::string peer) : m_peer(std::move(peer)) {}

  std::string m_peer;
  std::list<DMHistoryLine> m_lines;
};

struct UsersList : Message {
  UpdateType m_update_type{UpdateType::ManualRequest};

  void operator<<(const std::string& name) { m_users.push_back(name); }
  std::list<std::string> m_users;
};

struct ChannelsList : Message {
  UpdateType m_update_type{UpdateType::ManualRequest};

  void operator<<(const std::string& name) { m_channels.push_back(name); }
  std::list<std::string> m_channels;
};

struct SelfNameMessage : Message {
  explicit SelfNameMessage(std::string name) : m_name(std::move(name)) {}
  std::string m_name;
};

struct JoinedChannelMessage : Message {
  explicit JoinedChannelMessage(std::string channel)
      : m_channel(std::move(channel)) {}

  std::string m_channel;
};
