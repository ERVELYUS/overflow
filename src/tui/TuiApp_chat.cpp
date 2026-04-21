/*
  Chat-thread rendering for DMs and channels.
  This file renders the currently selected thread, applies per-thread scroll
  state, and keeps the bottom line pinned when autoscroll is enabled.
*/
#include "TuiApp.h"

Component TuiApp::make_chat_view(const WorkspaceState& state) {
  return Renderer([this, &state] {
    std::vector<Element> lines;

    const std::string active_name = get_active_item_name(state);
    const bool channels = (&state == &m_channels_state);

    const std::vector<ChatLine>* history = nullptr;
    ChatScrollState* scroll = nullptr;

    if (!active_name.empty()) {
      if (channels) {
        auto it = m_channel_histories.find(active_name);
        if (it != m_channel_histories.end()) {
          history = &it->second;
          scroll = &m_channel_scroll_states[active_name];
        }
      }
      else {
        auto it = m_dm_histories.find(active_name);
        if (it != m_dm_histories.end()) {
          history = &it->second;
          scroll = &m_dm_scroll_states[active_name];
        }
      }
    }

    if (history != nullptr && scroll != nullptr) {
      if (scroll->selected_line >= static_cast<int>(history->size())) {
        scroll->selected_line = static_cast<int>(history->size()) - 1;
      }
      if (scroll->selected_line < -1) {
        scroll->selected_line = -1;
      }

      const int last_index = static_cast<int>(history->size()) - 1;

      for (int i = 0; i < static_cast<int>(history->size()); ++i) {
        const auto& line = (*history)[i];

        Element row;
        if (line.system) {
          row = text(line.text) | dim;
        }
        else {
          Element sender = text(line.sender) |
                           color(line.sender == m_nickname ? Color::Red
                                                             : Color::Blue);
          row = hbox({sender, text(": " + line.text)});
        }

        // Keep the selected line in view without adding a visible highlight.
        if (scroll->follow_bottom && i == last_index) {
          row = row | focusPositionRelative(0.f, 1.f);
        }
        else if (!scroll->follow_bottom && i == scroll->selected_line) {
          row = row | focusPositionRelative(0.f, 0.5f);
        }

        lines.push_back(std::move(row));
      }
    }

    std::string display_title = active_name;
    if (m_tab_selected == to_index(TabEntry::DMs)) {
      display_title = "@" + active_name;
    }
    else if (m_tab_selected == to_index(TabEntry::Channels)) {
      display_title = "#" + active_name;
    }

    return vbox({
               text(" " + display_title + " ") | bold | center,
               separator(),
               filler(),
               vbox(std::move(lines)) | frame | vscroll_indicator,
           }) |
           flex;
  });
}
