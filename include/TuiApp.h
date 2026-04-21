#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Client.h"
#include "Message.h"

using namespace ftxui;

struct ChatLine {
  std::string sender;
  std::string text;
  bool system{false};
};

class TuiApp {
 public:
  TuiApp();
  void run(const std::string& ip, const std::string& port);

 private:
  enum class AppStage : int { Auth = 0, Main = 1 };
  static int to_index(AppStage stage) noexcept {
    return static_cast<int>(stage);
  }

  Client m_client;

  int m_stage{to_index(AppStage::Auth)};
  std::string m_auth_username{};
  std::string m_auth_password{};
  std::string m_auth_status{};

  Component m_auth_username_field;
  Component m_auth_password_field;

  Component make_auth_view();
  void submit_auth(bool register_flow);

  void handle_incoming_message(std::shared_ptr<Message> msg);

  enum class TabEntry : int { DMs = 0, Channels = 1 };
  enum class ViewMode : int { List = 0, Chat = 1 };
  static int to_index(TabEntry tab) noexcept { return static_cast<int>(tab); }
  static int to_index(ViewMode mode) noexcept { return static_cast<int>(mode); }

  struct WorkspaceState {
    int mode{to_index(ViewMode::List)};
    int active_item{-1};
    int selected_item{0};
    std::vector<std::string> items;
  };

  Component make_tab_header();
  Component make_section_view(std::string title);
  Component make_workspace(std::string menu_title, WorkspaceState& state);
  Component make_chat_view(const WorkspaceState& state);
  Element make_input_section();
  Component make_main_view();

  bool m_running{false};
  ScreenInteractive m_screen;
  std::string m_nickname;

  int m_modal_layer = 0;
  std::string m_new_channel_input;
  Component m_modal_input_field;
  Component make_create_channel_modal();
  std::vector<std::string> m_dms_display_items;
  std::vector<std::string> m_channels_display_items;

  struct ChatScrollState {
    int selected_line{-1};
    bool follow_bottom{true};
  };

  std::unordered_map<std::string, std::vector<ChatLine>> m_channel_histories;
  std::unordered_map<std::string, std::vector<ChatLine>> m_dm_histories;
  std::unordered_map<std::string, ChatScrollState> m_channel_scroll_states;
  std::unordered_map<std::string, ChatScrollState> m_dm_scroll_states;

  ChatScrollState& get_scroll_state(bool channels, const std::string& key);
  bool handle_chat_scroll_event(Event event);

  void append_line(std::vector<ChatLine>& history, const std::string& sender,
                   const std::string& text, bool system = false);

  int m_tab_selected{to_index(TabEntry::DMs)};
  const std::vector<std::string> m_tab_entries{" DMs ", " Channels "};

  bool m_in_chat{false};
  std::string m_input_buffer;
  Component m_input_field;

  WorkspaceState m_dms_state;
  WorkspaceState m_channels_state;
  static std::string get_active_item_name(const WorkspaceState& state) {
    if (state.active_item < 0 ||
        state.active_item >= static_cast<int>(state.items.size())) {
      return {};
    }
    return state.items[state.active_item];
  }
};
