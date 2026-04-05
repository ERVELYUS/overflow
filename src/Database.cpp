#include "Database.h"

#include <exception>
#include <iostream>

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

    m_db.exec(R"(
      CREATE TABLE IF NOT EXISTS users (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        username TEXT UNIQUE NOT NULL,
        created_at DATETIME DEFAULT CURRENT_TIMESTAMP
      );
    )");

    m_db.exec(R"(
      CREATE TABLE IF NOT EXISTS channels (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT UNIQUE,
        type INTEGER DEFAULT 0
      );
    )");

    m_db.exec(R"(
      CREATE TABLE IF NOT EXISTS channel_members (
        channel_id INTEGER NOT NULL,
        user_id INTEGER NOT NULL,
        PRIMARY KEY (channel_id, user_id),
        FOREIGN KEY (channel_id) REFERENCES channels (id) ON DELETE CASCADE,
        FOREIGN KEY (user_id) REFERENCES users (id) ON DELETE CASCADE
      );
    )");

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
    SQLite::Statement insert(m_db, "INSERT INTO users (username) VALUES (?)");
    insert.bind(1, username);
    insert.executeStep();
    return (int)m_db.getLastInsertRowid();
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
