#include "TuiApp.h"

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

namespace {
Component MakeSectionView(std::string title) {
  return Renderer([title = std::move(title)] {
    return vbox({
        text(title),
        separator(),
    });
  });
}

Component MakeTabHeader(int& tab_selected,
                        const std::vector<std::string>& tab_entries) {
  return Renderer([&tab_selected, &tab_entries] {
    std::vector<Element> tabs;
    tabs.reserve(tab_entries.size());

    for (int i = 0; i < static_cast<int>(tab_entries.size()); ++i) {
      auto tab = text(tab_entries[i]);
      if (i == tab_selected) {
        tab = tab | inverted | bold;
      }
      tabs.push_back(std::move(tab));
    }
    return hbox(std::move(tabs));
  });
}

Component MakeChannelsView(Component channels_menu) {
  return Renderer(channels_menu, [channels_menu] {
    return vbox({
        text(" AVAILABLE CHANNELS "),
        separator(),
        channels_menu->Render(),
        filler(),
        text("[ Press ENTER to join ]") | center,
    });
  });
}

Element MakeInputSection(Component input_field, bool in_chat) {
  if (!in_chat) {
    return text("");
  }
  return window(text(""), input_field->Render()) | size(HEIGHT, EQUAL, 3);
}
}  // namespace

TuiApp::TuiApp()
    : m_running(true), m_screen(ScreenInteractive::Fullscreen()) {

      };

void TuiApp::run() {
  int tab_selected = to_index(TabEntry::DMs);
  const std::vector<std::string> tab_entries = {" DMs ", " Channels ",
                                                " Settings "};

  auto tab_header = MakeTabHeader(tab_selected, tab_entries);

  int channel_selected = 0;
  std::vector<std::string> channels_list = {
      "#general",
      "#development",
      "#random",
  };
  auto channels_menu = Menu(&channels_list, &channel_selected);

  std::string input_buffer;
  auto input_field = Input(&input_buffer, "type here...");
  bool in_chat = false;

  auto dms_view = MakeSectionView(" ONLINE USERS ");
  auto channels_view = MakeChannelsView(channels_menu);
  auto settings_view = MakeSectionView(" SETTINGS ");

  auto tab_container = Container::Tab(
      {
          dms_view,
          channels_view,
          settings_view,
      },
      &tab_selected);

  auto main_container = Container::Vertical({
      tab_header,
      tab_container,
      input_field,
  });

  auto event_handler = CatchEvent(main_container, [&](Event event) {
    if (event == Event::F1) {
      tab_selected = to_index(TabEntry::DMs);
      tab_header->TakeFocus();
      return true;
    }

    if (event == Event::F2) {
      tab_selected = to_index(TabEntry::Channels);
      tab_header->TakeFocus();
      return true;
    }

    if (event == Event::F12) {
      tab_selected = to_index(TabEntry::Settings);
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
    return vbox({
        window(hbox({tab_header->Render(), filler()}),
               tab_container->Render() | flex) |
            flex,
        MakeInputSection(input_field, in_chat),
    });
  });

  m_screen.Loop(main_renderer);
}
