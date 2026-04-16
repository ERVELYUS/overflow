#pragma once

#include <list>
#include <string>

#include "Console.h"

struct Message {
  virtual ~Message() = default;  // RTTI
};

struct SystemMessage : public Message {
  ConsoleLevel m_level;
  std::string m_text;
  SystemMessage(ConsoleLevel l, std::string t)
      : m_level(l), m_text(std::move(t)) {}
};

struct UserMessage : public Message {
  UserMessage(const std::string& name, const std::string& msg)
      : m_name(name), m_msg(msg) {}
  std::string m_name;
  std::string m_msg;
};

struct UsersList : public Message {
  void operator<<(const std::string& name) { m_users.push_back(name); }
  std::list<std::string> m_users;
};

struct ChannelsList : public Message {
  void operator<<(const std::string& name) { m_channels.push_back(name); }
  std::list<std::string> m_channels;
};

struct SelfNameMessage : public Message {
  SelfNameMessage(const std::string& name) : m_name(name) {}
  std::string m_name;
};
