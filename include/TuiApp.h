#pragma once
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

class TuiApp {
 public:
  TuiApp();
  void run();

 private:
  enum class ChatKind { None, DM, Channel };
  enum class TabEntry : int { DMs = 0, Channels = 1, Settings = 2 };
  enum class ChannelMode : int { List = 0, Chat = 1 };
  static int to_index(TabEntry tab) noexcept { return static_cast<int>(tab); };
  static int to_index(ChannelMode mode) noexcept {
    return static_cast<int>(mode);
  };

  Component MakeTabHeader();
  Component MakeSectionView(std::string title);
  Component MakeChannelsWorkspace();
  Component MakeChatView(const std::string& title,
                         const std::vector<std::string>& messages);
  Element MakeInputSection();

  bool m_running{false};
  ScreenInteractive m_screen;

  int m_tab_selected{to_index(TabEntry::DMs)};
  const std::vector<std::string> m_tab_entries{" DMs ", " Channels ",
                                               " Settings "};

  bool m_in_chat{false};
  std::string m_input_buffer;
  Component m_input_field;

  int m_channel_mode{to_index(ChannelMode::List)};
  int m_active_channel{-1};
  int m_channel_selected{0};

  // Dummy data
  std::vector<std::string> m_channels_list{"#general", "#development",
                                           "#random"};
  std::vector<std::string> m_dummy_msgs{"User1: Hello", "User2: Hello back"};
};
