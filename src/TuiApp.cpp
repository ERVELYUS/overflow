#include "TuiApp.h"

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

TuiApp::TuiApp()
    : m_running(true), m_screen(ScreenInteractive::Fullscreen()) {

      };

void TuiApp::run() {
  // Switcher between DMs, Channels and Settings (in the future)
  int tab_selected = 0;  // Start on "Users"
  std::vector<std::string> tab_entries = {" DMs ", " Channels ", " Settings "};
  auto tab_header = Renderer([&] {
    std::vector<Element> tabs;

    for (int i = 0; i < tab_entries.size(); ++i) {
      auto tab = text(tab_entries[i]);

      if (i == tab_selected) {
        tab = tab | inverted | bold;
      }

      tabs.push_back(tab);
    }

    return hbox(std::move(tabs));
  });

  // channels_list will be filled from server later
  int channel_selected = 0;
  std::vector<std::string> channels_list = {
      "#general", "#development",  // Random list for tab switching test
      "#random"};
  auto channels_menu = Menu(&channels_list, &channel_selected);

  // Input field
  std::string input_buffer = "";
  auto input_field = Input(&input_buffer, "type here...");

  // The Tab Container holds the logic for switching views
  auto tab_container = Container::Tab(
      {
          Renderer([&] { return vbox({text(" ONLINE USERS "), separator()}); }),
          Renderer(channels_menu,
                   [&] {
                     return vbox({text(" AVAILABLE CHANNELS "), separator(),
                                  channels_menu->Render(), filler(),
                                  text("[ Press ENTER to Join ]") | center});
                   }),
          Renderer([&] { return vbox({text(" SETTINGS "), separator()}); }),
      },
      &tab_selected);

  // Container::Vertical for tab management (feels wonky for now)
  auto main_container = Container::Vertical({
      tab_header,
      tab_container,
      input_field,
  });

  auto event_handler = CatchEvent(main_container, [&](Event event) {
    if (event == Event::F1) {  // DMs tab
      tab_selected = 0;
      tab_header->TakeFocus();
      return true;
    }
    if (event == Event::F2) {  // Channels tab
      tab_selected = 1;
      tab_header->TakeFocus();
      return true;
    }
    if (event == Event::F12) {  // Settings tab
      tab_selected = 2;
      tab_header->TakeFocus();
      return true;
    }

    if (event == Event::CtrlQ) {
      m_screen.Exit();
      return true;
    }
    return false;
  });

  auto main_renderer = Renderer(event_handler, [&] {
    return vbox(
        {window(hbox({tab_header->Render(), filler()}),
                tab_container->Render() | flex) |
             flex,
         window(text(""), input_field->Render()) | size(HEIGHT, EQUAL, 3)});
  });

  m_screen.Loop(main_renderer);
}
