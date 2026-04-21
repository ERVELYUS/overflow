/*
  TuiApp bootstrap and shared state helpers.
  This file wires up the persistent input widgets and the small helper methods
  that are reused by the auth flow, message routing, and chat views.
*/
#include "TuiApp.h"

#include <algorithm>

TuiApp::TuiApp() : m_running(true), m_screen(ScreenInteractive::Fullscreen()) {
  m_channels_state.items.clear();
  m_dms_state.items.clear();

  m_modal_input_field = Input(&m_new_channel_input, "New channel name...");

  // Main chat input: also handles chat scrolling keys so scrolling works even
  // when the text input owns focus.
  auto input_base = Input(&m_input_buffer, "type here...");
  m_input_field = CatchEvent(input_base, [this](Event event) {
    if (handle_chat_scroll_event(event)) {
      return true;
    }

    if (event == Event::Return && !m_input_buffer.empty()) {
      m_client.send_message(m_input_buffer);

      if (m_tab_selected == to_index(TabEntry::Channels)) {
        const std::string key = get_active_item_name(m_channels_state);
        if (!key.empty()) {
          auto& history = m_channel_histories[key];
          append_line(history, m_nickname, m_input_buffer);

          auto& scroll = get_scroll_state(true, key);
          if (scroll.follow_bottom) {
            scroll.selected_line = static_cast<int>(history.size()) - 1;
          }
        }
      }
      else if (m_tab_selected == to_index(TabEntry::DMs)) {
        const std::string key = get_active_item_name(m_dms_state);
        if (!key.empty()) {
          m_client.send_message("/pm " + key + " " + m_input_buffer);

          auto& history = m_dm_histories[key];
          append_line(history, m_nickname, m_input_buffer);

          auto& scroll = get_scroll_state(false, key);
          if (scroll.follow_bottom) {
            scroll.selected_line = static_cast<int>(history.size()) - 1;
          }
        }
      }

      m_input_buffer.clear();
      return true;
    }
    return false;
  });

  // Auth inputs
  m_auth_username_field = Input(&m_auth_username, "nickname...");
  m_auth_password_field = Input(&m_auth_password, "password...");
}

TuiApp::ChatScrollState& TuiApp::get_scroll_state(bool channels,
                                                  const std::string& key) {
  return channels ? m_channel_scroll_states[key] : m_dm_scroll_states[key];
}

bool TuiApp::handle_chat_scroll_event(Event event) {
  if (!m_in_chat) {
    return false;
  }

  const bool in_channels = m_tab_selected == to_index(TabEntry::Channels);
  const bool in_dms = m_tab_selected == to_index(TabEntry::DMs);
  if (!in_channels && !in_dms) {
    return false;
  }

  const std::string active_name = in_channels
                                      ? get_active_item_name(m_channels_state)
                                      : get_active_item_name(m_dms_state);
  if (active_name.empty()) {
    return false;
  }

  auto& history = in_channels ? m_channel_histories[active_name]
                              : m_dm_histories[active_name];
  auto& scroll = get_scroll_state(in_channels, active_name);

  const int last = static_cast<int>(history.size()) - 1;
  if (last < 0) {
    return false;
  }

  auto clamp_index = [last](int value) { return std::clamp(value, 0, last); };

  if (event == Event::Home) {
    scroll.selected_line = 0;
    scroll.follow_bottom = false;
    return true;
  }
  if (event == Event::End) {
    scroll.selected_line = last;
    scroll.follow_bottom = true;
    return true;
  }
  if (event == Event::PageUp) {
    scroll.selected_line = clamp_index(scroll.selected_line - 10);
    scroll.follow_bottom = false;
    return true;
  }
  if (event == Event::PageDown) {
    scroll.selected_line = clamp_index(scroll.selected_line + 10);
    scroll.follow_bottom = (scroll.selected_line == last);
    return true;
  }

  return false;
}

void TuiApp::append_line(std::vector<ChatLine>& history,
                         const std::string& sender, const std::string& text,
                         bool system) {
  history.push_back(ChatLine{sender, text, system});
}
