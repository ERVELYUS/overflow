#pragma once
#include <SQLiteCpp/SQLiteCpp.h>

#include <optional>
#include <string>
#include <vector>

struct UserRecord {
  int id;
  std::string username;
};

enum class ChannelType {
  Public,
  DM,
};

struct ChannelRecord {
  int id;
  std::string name;
  ChannelType type;
};

struct MessageRecord {
  int id;
  int channel_id;
  int sender_id;
  std::string sender_name;
  std::string content;
  std::string timestamp;
};

class Database {
  SQLite::Database m_db;

 public:
  explicit Database(const std::string& db_path);

  void initialize_schema();

  // Users
  int get_or_create_user(const std::string& username);
  std::optional<std::string> get_username(int user_id);

  // Channels/DMs
  int get_or_create_channel(const std::string& name);
  int get_or_create_dm(int user1_id, int user2_id);
  std::vector<ChannelRecord> get_all_public_channels();
  void add_user_to_channel(int channel_id, int user_id);

  // Messages
  void save_message(int channel_id, int sender_id, const std::string& content);

  std::vector<MessageRecord> get_recent_messages(int channel_id,
                                                 int limit = 50);
};
