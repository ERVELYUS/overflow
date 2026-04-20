#pragma once
#include <SQLiteCpp/SQLiteCpp.h>

#include <optional>
#include <string>
#include <vector>

struct UserRecord {
  int id;
  std::string username;
};

struct DMRecord {
  int channel_id;
  std::string peer_name;
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
  bool username_exists(const std::string& username);
  void update_username(int user_id, const std::string& new_name);
  bool register_user(const std::string& username, const std::string& password);
  std::optional<int> authenticate_user(const std::string& username,
                                       const std::string& password);

  // Channels/DMs
  int get_or_create_channel(const std::string& name);
  int get_or_create_dm(int user1_id, int user2_id);
  std::vector<ChannelRecord> get_all_channels();
  void add_user_to_channel(int channel_id, int user_id);
  void remove_user_from_channel(int channel_id, int user_id);

  // Messages
  void save_message(int channel_id, int sender_id, const std::string& content);

  std::vector<MessageRecord> get_recent_messages(int channel_id,
                                                 int limit = 50);
  std::vector<DMRecord> get_dm_for_user(int user_id);
};
