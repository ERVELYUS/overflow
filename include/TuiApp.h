#pragma once
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "Client.h"
#include "Message.h"

using namespace ftxui;

class TuiApp {
 public:
  TuiApp();
  void run();

 private:
  Client m_client;
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

  bool m_running{false};
  ScreenInteractive m_screen;
  std::string m_nickname{"Me"};

  // Internal state
  int m_tab_selected{to_index(TabEntry::DMs)};
  const std::vector<std::string> m_tab_entries{" DMs ", " Channels ",
                                               " Settings "};

  bool m_in_chat{false};
  std::string m_input_buffer;
  Component m_input_field;

  WorkspaceState m_dms_state;
  WorkspaceState m_channels_state;

  // Dummy data
  std::vector<std::string> m_dummy_msgs{"User1: Hello", "User2: Hello back"};
};
