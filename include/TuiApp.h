#pragma once
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

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
  enum class AppStage : int {
    Auth = 0,
    Main = 1,
  };
  static int to_index(AppStage stage) noexcept {
    return static_cast<int>(stage);
  };

  Client m_client;

  int m_stage{to_index(AppStage::Auth)};
  std::string m_auth_username{};
  std::string m_auth_password{};
  std::string m_auth_status{};

  Component m_auth_username_field;
  Component m_auth_password_field;

  Component MakeAuthView();
  void SubmitAuth(bool register_flow);

  void HandleIncomingMessage(std::shared_ptr<Message> msg);

  // Enums and how they work
  enum class TabEntry : int { DMs = 0, Channels = 1, Settings = 2 };
  enum class ViewMode : int { List = 0, Chat = 1 };
  static int to_index(TabEntry tab) noexcept { return static_cast<int>(tab); };
  static int to_index(ViewMode mode) noexcept {
    return static_cast<int>(mode);
  };

  struct WorkspaceState {
    int mode{to_index(ViewMode::List)};
    int active_item{-1};
    int selected_item{0};
    std::vector<std::string> items;
  };

  // Components and elements
  Component MakeTabHeader();
  Component MakeSectionView(std::string title);
  Component MakeWorkspace(std::string menu_title, WorkspaceState& state);
  Component MakeChatView(const WorkspaceState& state);
  Element MakeInputSection();
  Component MakeMainView();

  bool m_running{false};
  ScreenInteractive m_screen;
  std::string m_nickname{""};

  // Channel creation
  int m_modal_layer = 0;
  std::string m_new_channel_input;
  Component m_modal_input_field;
  Component MakeCreateChannelModal();
  std::vector<std::string> m_dms_display_items;
  std::vector<std::string> m_channels_display_items;

  std::unordered_map<std::string, std::vector<ChatLine>> m_channel_histories;
  std::unordered_map<std::string, std::vector<ChatLine>> m_dm_histories;
  void AppendLine(std::vector<ChatLine>& history, const std::string& sender,
                  const std::string& text, bool system = false);

  // Internal state
  int m_tab_selected{to_index(TabEntry::DMs)};
  const std::vector<std::string> m_tab_entries{" DMs ", " Channels ",
                                               " Settings "};

  bool m_in_chat{false};
  std::string m_input_buffer;
  Component m_input_field;

  WorkspaceState m_dms_state;
  WorkspaceState m_channels_state;
  std::string GetActiveItemName(const TuiApp::WorkspaceState& state) {
    if (state.active_item < 0 ||
        state.active_item >= static_cast<int>(state.items.size())) {
      return {};
    }
    return state.items[state.active_item];
  }
};
