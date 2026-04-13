#include "Database.h"

#include <bcrypt/bcrypt.h>

#include <algorithm>
#include <exception>
#include <iostream>

#include "bcrypt/BCrypt.hpp"

Database::Database(const std::string& db_path)
    : m_db(db_path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE |
                        SQLite::OPEN_FULLMUTEX) {
  m_db.exec("PRAGMA foreign_keys = ON;");
  std::cout << "[Database] Successfully opened: " << db_path << "\n";
};

void Database::initialize_schema() {
  try {
    m_db.exec(
        "CREATE TABLE IF NOT EXISTS schema_version (version INTEGER PRIMARY "
        "KEY);");

    // USERS TABLE
    m_db.exec(R"(
      CREATE TABLE IF NOT EXISTS users (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        username TEXT UNIQUE NOT NULL,
        password_hash TEXT NOT NULL,
        created_at DATETIME DEFAULT CURRENT_TIMESTAMP
      );
    )");

    // CHANNELS TABLE
    m_db.exec(R"(
      CREATE TABLE IF NOT EXISTS channels (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT UNIQUE,
        type INTEGER DEFAULT 0
      );
    )");

    // CHANNELS_MEMBERS TABLE
    m_db.exec(R"(
      CREATE TABLE IF NOT EXISTS channel_members (
        channel_id INTEGER NOT NULL,
        user_id INTEGER NOT NULL,
        PRIMARY KEY (channel_id, user_id),
        FOREIGN KEY (channel_id) REFERENCES channels (id) ON DELETE CASCADE,
        FOREIGN KEY (user_id) REFERENCES users (id) ON DELETE CASCADE
      );
    )");

    // MESSAGES TABLE
    m_db.exec(R"(
      CREATE TABLE IF NOT EXISTS messages (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        channel_id INTEGER NOT NULL,
        sender_id INTEGER NOT NULL,
        content TEXT NOT NULL,
        timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
        FOREIGN KEY (channel_id) REFERENCES channels (id) ON DELETE CASCADE,
        FOREIGN KEY (sender_id) REFERENCES users (id) ON DELETE CASCADE
      );
    )");

    // Speeds up loading chat history for a specific channel
    m_db.exec(
        "CREATE INDEX IF NOT EXISTS idx_messages_channel_id ON messages "
        "(channel_id);");

    // Speeds up finding which channels a specific user belongs to
    m_db.exec(
        "CREATE INDEX IF NOT EXISTS idx_channel_members_user_id ON "
        "channel_members (user_id);");

    std::cout << "[Database] Foundation tables initialized.\n";
  }
  catch (std::exception& e) {
    std::cerr << "[Database] Error initializing schema: " << e.what() << "\n";
  }
};

int Database::get_or_create_user(const std::string& username) {
  SQLite::Statement query(m_db, "SELECT id FROM users WHERE username = ?");
  query.bind(1, username);

  if (query.executeStep()) {
    return query.getColumn(0).getInt();
  }

  // Race condition protection
  try {
    SQLite::Statement insert_query(m_db,
                                   "INSERT INTO users (username) VALUES (?)");
    insert_query.bind(1, username);
    insert_query.exec();
    return static_cast<int>(m_db.getLastInsertRowid());
  }
  catch (std::exception& e) {
    query.reset();
    query.bind(1, username);
    if (query.executeStep()) {
      return query.getColumn(0).getInt();
    }
    throw;
  }
};

int Database::get_or_create_channel(const std::string& name) {
  SQLite::Statement lookup_query(
      m_db, "SELECT id FROM channels WHERE name = ? AND type = 0");
  lookup_query.bind(1, name);

  if (lookup_query.executeStep()) {
    return lookup_query.getColumn(0).getInt();
  }

  // Race condition protection
  try {
    SQLite::Statement insert_query(
        m_db, "INSERT INTO channels (name, type) VALUES (?, ?)");
    insert_query.bind(1, name);
    insert_query.bind(2, 0);
    insert_query.exec();
    return static_cast<int>(m_db.getLastInsertRowid());
  }
  catch (std::exception& e) {
    lookup_query.reset();
    lookup_query.bind(1, name);
    if (lookup_query.executeStep()) {
      return lookup_query.getColumn(0).getInt();
    }
    throw;
  }
};

int Database::get_or_create_dm(int user1_id, int user2_id) {
  SQLite::Statement lookup_query(
      m_db,
      "SELECT channel_id FROM channel_members WHERE user_id IN (?, ?) GROUP BY "
      "channel_id HAVING COUNT(user_id) = 2;");
  lookup_query.bind(1, user1_id);
  lookup_query.bind(2, user2_id);

  if (lookup_query.executeStep()) {
    return lookup_query.getColumn(0).getInt();
  }

  // Race condition protection
  try {
    SQLite::Statement insert_channel_query(
        m_db, "INSERT INTO channels (name, type) VALUES (NULL, 1)");
    insert_channel_query.exec();
    int created_channel_id = static_cast<int>(m_db.getLastInsertRowid());

    add_user_to_channel(created_channel_id, user1_id);
    add_user_to_channel(created_channel_id, user2_id);

    return created_channel_id;
  }
  catch (std::exception& e) {
    lookup_query.reset();
    lookup_query.bind(1, user1_id);
    lookup_query.bind(2, user2_id);
    if (lookup_query.executeStep()) {
      return lookup_query.getColumn(0).getInt();
    }
    throw;
  }
}

void Database::add_user_to_channel(int channel_id, int user_id) {
  SQLite::Statement query(m_db,
                          "INSERT OR IGNORE INTO channel_members (channel_id, "
                          "user_id) VALUES (?, ?)");
  query.bind(1, channel_id);
  query.bind(2, user_id);
  query.exec();
}

// TODO: add something other than std::string
void Database::save_message(int channel_id, int sender_id,
                            const std::string& content) {
  SQLite::Statement query(
      m_db,
      "INSERT INTO messages (channel_id, sender_id, content) VALUES (?, ?, ?)");
  query.bind(1, channel_id);
  query.bind(2, sender_id);
  query.bind(3, content);
  query.exec();
}

std::vector<MessageRecord> Database::get_recent_messages(int channel_id,
                                                         int limit) {
  std::vector<MessageRecord> messages;

  SQLite::Statement query(m_db, R"(
    SELECT m.id, m.channel_id, m.sender_id, u.username, m.content, m.timestamp
    FROM messages m
    JOIN users u ON m.sender_id = u.id
    WHERE m.channel_id = ?
    ORDER BY m.timestamp DESC
    LIMIT ?
  )");
  query.bind(1, channel_id);
  query.bind(2, limit);

  while (query.executeStep()) {
    MessageRecord msg;
    msg.id = query.getColumn(0).getInt();
    msg.channel_id = query.getColumn(1).getInt();
    msg.sender_id = query.getColumn(2).getInt();
    msg.sender_name = query.getColumn(3).getText();
    msg.content = query.getColumn(4).getText();
    msg.timestamp = query.getColumn(5).getText();

    messages.push_back(msg);
  }

  std::reverse(messages.begin(), messages.end());

  return messages;
};

std::optional<std::string> Database::get_username(int user_id) {
  SQLite::Statement query(m_db, "SELECT username FROM users WHERE id = ?");
  query.bind(1, user_id);
  if (query.executeStep()) {
    return query.getColumn(0).getText();
  }
  return std::nullopt;
};

bool Database::register_user(const std::string& username,
                             const std::string& password) {
  try {
    std::string hashed_password = BCrypt::generateHash(password, 12);

    SQLite::Statement query(
        m_db, "INSERT INTO users (username, password_hash) VALUES (?, ?)");
    query.bind(1, username);
    query.bind(2, hashed_password);
    query.exec();
    return true;
  }
  catch (...) {
    return false;
  }
};

std::optional<int> Database::authenticate_user(const std::string& username,
                                               const std::string& password) {
  SQLite::Statement query(
      m_db, "SELECT id, password_hash FROM users WHERE username = ?");
  query.bind(1, username);
  if (query.executeStep()) {
    int user_id = query.getColumn(0).getInt();
    std::string stored_hash = query.getColumn(1).getText();

    if (BCrypt::validatePassword(password, stored_hash)) {
      return user_id;
    }
  }
  return std::nullopt;
}

std::vector<ChannelRecord> Database::get_all_public_channels() {
  std::vector<ChannelRecord> channels;
  SQLite::Statement query(m_db, R"(
    SELECT id, name
    FROM channels 
    WHERE type = 0
  )");
  while (query.executeStep()) {
    channels.push_back({query.getColumn(0).getInt(),
                        query.getColumn(1).getText(), ChannelType::Public});
  }
  return channels;
}
