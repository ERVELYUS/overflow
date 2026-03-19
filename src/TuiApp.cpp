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
  auto tab_header = Toggle(&tab_entries, &tab_selected);

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

  auto main_renderer = Renderer(main_container, [&] {
    auto content_window =
        window(hbox({tab_header->Render(), filler()}),  // Tabs in the border
               tab_container->Render() | flex  // Content takes all space
               ) |
        flex;

    auto input_window =
        window(text(""), input_field->Render()) | size(HEIGHT, EQUAL, 3);

    return vbox({content_window, input_window});
  });

  m_screen.Loop(main_renderer);
}
