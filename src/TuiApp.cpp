#include "TuiApp.h"

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

TuiApp::TuiApp()
    : m_running(true), m_screen(ScreenInteractive::Fullscreen()) {};

namespace {
// A dud to later use for DMs and Settings
Component MakeSectionView(std::string title) {
  return Renderer([title = std::move(title)] {
    return vbox({
        text(title),
        separator(),
    });
  });
}

// How do tabs look
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

// How does the channels view panel look
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

// How does input input field looks and when it's shown
Element MakeInputSection(Component input_field, bool in_chat) {
  if (!in_chat) {
    return text("");
  }
  return window(text(""), input_field->Render()) | size(HEIGHT, EQUAL, 3);
}

Component MakeChatView(const int& active_channel,
                       const std::vector<std::string>& channels_list,
                       const std::vector<std::string>& messages) {
  return Renderer([&active_channel, &channels_list, &messages] {
    if (active_channel < 0 ||
        active_channel >= static_cast<int>(channels_list.size())) {
      return text("No channel selected") | center;
    }

    std::vector<Element> lines;
    lines.reserve(messages.size());
    for (auto const& msg : messages) {
      lines.push_back(text(msg));
    }

    return vbox({
        text(channels_list[active_channel]) | bold | center,
        separator(),
        vbox(std::move(lines)) | vscroll_indicator | flex,
    });
  });
}
}  // namespace

void TuiApp::run() {
  bool in_chat = false;
  int active_channel = -1;
  int tab_selected = to_index(TabEntry::DMs);
  const std::vector<std::string> tab_entries = {" DMs ", " Channels ",
                                                " Settings "};
  auto tab_header = MakeTabHeader(tab_selected, tab_entries);
  std::string input_buffer;
  auto input_field = Input(&input_buffer, "type here...");

  // Dummy data, will be replaced with actual list once server is wired
  int channel_selected = 0;
  std::vector<std::string> channels_list = {
      "#general",
      "#development",
      "#random",
  };

  int channel_mode = 0;
  std::vector<std::string> dummy_msgs = {"User1: Hello", "User2: Hello back"};

  MenuOption option;
  option.on_enter = [&] {
    active_channel = channel_selected;
    channel_mode = 1;
    in_chat = true;
    input_field->TakeFocus();
  };
  auto channels_menu = Menu(&channels_list, &channel_selected, option);

  auto dms_view = MakeSectionView(" ONLINE USERS ");
  auto channels_view = MakeChannelsView(channels_menu);
  auto settings_view = MakeSectionView(" SETTINGS ");
  auto chat_view = MakeChatView(active_channel, channels_list, dummy_msgs);

  auto channels_router =
      Container::Tab({channels_view, chat_view}, &channel_mode);

  auto channels_workspace = CatchEvent(channels_router, [&](Event event) {
    if (channel_mode == 1 && event == Event::Escape) {
      channel_mode = 0;
      in_chat = false;
      channels_menu->TakeFocus();
      return true;
    }
    return false;
  });

  auto tab_container = Container::Tab(
      {
          dms_view,
          channels_workspace,
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
