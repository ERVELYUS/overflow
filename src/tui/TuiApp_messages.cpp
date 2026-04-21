/*
  Server-to-UI message dispatcher.
  This file converts protocol messages into local TUI state updates: login,
  channel lists, DM history restoration, and live chat traffic.
*/
#include "TuiApp.h"

#include <algorithm>

void TuiApp::submit_auth(bool register_flow) {
  if (m_auth_username.empty() || m_auth_password.empty()) {
    m_auth_status = "Please enter both nickname and password.";
    return;
  }

  m_auth_status = register_flow ? "Registering..." : "Logging in...";
  const std::string command = (register_flow ? "/register " : "/login ") +
                              m_auth_username + " " + m_auth_password;

  m_client.send_message(command);
}

void TuiApp::handle_incoming_message(std::shared_ptr<Message> msg) {
  if (auto sysMsg = std::dynamic_pointer_cast<SystemMessage>(msg)) {
    if (m_stage == to_index(AppStage::Auth)) {
      m_auth_status = sysMsg->m_text;
    }
    else {
      const std::string target = get_active_item_name(
          m_tab_selected == to_index(TabEntry::Channels) ? m_channels_state
                                                         : m_dms_state);

      if (!target.empty()) {
        auto& history = (m_tab_selected == to_index(TabEntry::Channels))
                            ? m_channel_histories[target]
                            : m_dm_histories[target];

        append_line(history, "System", sysMsg->m_text, true);
      }
    }
  }
  else if (auto userMsg = std::dynamic_pointer_cast<UserMessage>(msg)) {
    if (m_tab_selected == to_index(TabEntry::Channels)) {
      const std::string key = get_active_item_name(m_channels_state);
      if (!key.empty()) {
        auto& history = m_channel_histories[key];
        append_line(history, userMsg->m_name, userMsg->m_msg);

        auto& scroll = get_scroll_state(true, key);
        if (scroll.follow_bottom) {
          scroll.selected_line = static_cast<int>(history.size()) - 1;
        }
      }
    }
    else {
      const std::string key = userMsg->m_name;
      auto& history = m_dm_histories[key];
      append_line(history, userMsg->m_name, userMsg->m_msg);

      auto& scroll = get_scroll_state(false, key);
      if (scroll.follow_bottom) {
        scroll.selected_line = static_cast<int>(history.size()) - 1;
      }
    }
  }
  else if (auto dmHistory = std::dynamic_pointer_cast<DMHistoryMessage>(msg)) {
    auto& history = m_dm_histories[dmHistory->m_peer];
    history.clear();

    for (const auto& line : dmHistory->m_lines) {
      append_line(history, line.m_sender, line.m_text);
    }

    auto& scroll = get_scroll_state(false, dmHistory->m_peer);
    scroll.follow_bottom = true;
    scroll.selected_line = history.empty() ? -1
                                           : static_cast<int>(history.size()) - 1;

    if (std::find(m_dms_state.items.begin(), m_dms_state.items.end(),
                  dmHistory->m_peer) == m_dms_state.items.end()) {
      m_dms_state.items.push_back(dmHistory->m_peer);
      m_dms_display_items.push_back("@" + dmHistory->m_peer);
    }
  }
  else if (auto usersList = std::dynamic_pointer_cast<UsersList>(msg)) {
    m_dms_state.items.clear();
    m_dms_display_items.clear();

    for (const auto& user : usersList->m_users) {
      if (user == m_nickname) continue;
      m_dms_state.items.push_back(user);
      m_dms_display_items.push_back("@" + user);
    }
  }
  else if (auto channelsList = std::dynamic_pointer_cast<ChannelsList>(msg)) {
    m_channels_state.items.clear();
    m_channels_display_items.clear();

    for (const auto& chan : channelsList->m_channels) {
      m_channels_state.items.push_back(chan);
      m_channels_display_items.push_back("#" + chan);
    }
  }
  else if (auto selfMsg = std::dynamic_pointer_cast<SelfNameMessage>(msg)) {
    m_nickname = selfMsg->m_name;

    if (m_stage == to_index(AppStage::Auth)) {
      m_stage = to_index(AppStage::Main);
      m_auth_status = "Authenticated as " + m_nickname;
      m_auth_password.clear();
      m_in_chat = false;

      m_client.send_message("/channels");
      m_client.send_message("/users");
    }
  }
  else if (auto joinedMsg =
               std::dynamic_pointer_cast<JoinedChannelMessage>(msg)) {
    const std::string& channel = joinedMsg->m_channel;

    m_stage = to_index(AppStage::Main);
    m_in_chat = true;

    (void)m_channel_histories[channel];

    m_tab_selected = to_index(TabEntry::Channels);
    auto it = std::find(m_channels_state.items.begin(),
                        m_channels_state.items.end(), channel);
    if (it != m_channels_state.items.end()) {
      m_channels_state.active_item =
          static_cast<int>(std::distance(m_channels_state.items.begin(), it));
    }

    auto& scroll = get_scroll_state(true, channel);
    scroll.follow_bottom = true;
    scroll.selected_line = m_channel_histories[channel].empty()
                               ? -1
                               : static_cast<int>(m_channel_histories[channel].size()) - 1;

    m_auth_status.clear();
  }
}
