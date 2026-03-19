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
  bool m_running{false};
  ScreenInteractive m_screen;
};
