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
  enum class TabEntry : int {
    DMs = 0,
    Channels = 1,
    Settings = 2,
  };
  static int to_index(TabEntry tab);

  bool m_running{false};
  ScreenInteractive m_screen;
};
