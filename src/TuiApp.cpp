#include "TuiApp.h"

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

TuiApp::TuiApp()
    : m_running(true),
      m_screen(ScreenInteractive::Fullscreen()),
      m_input_field(Input(&m_input_buffer, "type here...")) {};

// A dud to later use for DMs and Settings
Component TuiApp::MakeSectionView(std::string title) {
  return Renderer([title = std::move(title)] {
    return vbox({
        text(title),
        separator(),
    });
  });
}

// How do tabs look
Component TuiApp::MakeTabHeader() {
  return Renderer([this] {
    std::vector<Element> tabs;
    tabs.reserve(m_tab_entries.size());

    for (int i = 0; i < static_cast<int>(m_tab_entries.size()); ++i) {
      auto tab = text(m_tab_entries[i]);
      if (i == m_tab_selected) {
        tab = tab | inverted | bold;
      }
      tabs.push_back(std::move(tab));
    }
    return hbox(std::move(tabs));
  });
}

// How does the chat room look
Component TuiApp::MakeChatView(const std::string& title,
                               const std::vector<std::string>& messages) {
  return Renderer([this, title] {
    std::vector<Element> lines;
    for (auto const& msg : m_dummy_msgs) {
      lines.push_back(text(msg));
    }

    std::string display_title = title;
    if (m_active_channel >= 0 && m_active_channel < m_channels_list.size()) {
      display_title = m_channels_list[m_active_channel];
    }

    return vbox({text(" " + display_title + " ") | bold | center, separator(),
                 filler(), vbox(std::move(lines)) | vscroll_indicator}) |
           flex;
  });
}

// Builds the channels menu, the chat view, and the logic to swap between them
Component TuiApp::MakeChannelsWorkspace() {
  MenuOption option;
  option.on_enter = [this] {
    m_active_channel = m_channel_selected;
    m_channel_mode = to_index(ChannelMode::Chat);
    m_in_chat = true;
    m_input_field->TakeFocus();
  };

  auto channels_menu = Menu(&m_channels_list, &m_channel_selected, option);

  auto channels_list_view = Renderer(channels_menu, [channels_menu] {
    return vbox({
        text(" AVAILABLE CHANNELS "),
        separator(),
        channels_menu->Render(),
        filler(),
        text("[ Press ENTER to join ]") | center,
    });
  });

  std::string current_title =
      (m_active_channel >= 0) ? m_channels_list[m_active_channel] : "";
  auto chat_view = MakeChatView(current_title, m_dummy_msgs);

  auto channels_router =
      Container::Tab({channels_list_view, chat_view}, &m_channel_mode);

  return CatchEvent(channels_router, [this, channels_menu](Event event) {
    if (m_channel_mode == to_index(ChannelMode::Chat) &&
        event == Event::Escape) {
      m_channel_mode = to_index(ChannelMode::List);
      m_in_chat = false;
      channels_menu->TakeFocus();
      return true;
    }
    return false;
  });
}

// How does input field look and when it's shown
Element TuiApp::MakeInputSection() {
  if (!m_in_chat) {
    return text("");
  }
  return window(text(""), m_input_field->Render()) | size(HEIGHT, EQUAL, 3);
}

void TuiApp::run() {
  auto tab_header = MakeTabHeader();
  auto dms_view = MakeSectionView(" ONLINE USERS ");
  auto channels_workspace = MakeChannelsWorkspace();
  auto settings_view = MakeSectionView(" SETTINGS ");

  auto tab_container = Container::Tab(
      {dms_view, channels_workspace, settings_view}, &m_tab_selected);

  auto main_container = Container::Vertical({
      tab_header,
      tab_container,
      m_input_field,
  });

  auto event_handler = CatchEvent(main_container, [&](Event event) {
    if (event == Event::F1) {
      m_tab_selected = to_index(TabEntry::DMs);
      tab_header->TakeFocus();
      return true;
    }
    if (event == Event::F2) {
      m_tab_selected = to_index(TabEntry::Channels);
      tab_header->TakeFocus();
      return true;
    }
    if (event == Event::F12) {
      m_tab_selected = to_index(TabEntry::Settings);
      tab_header->TakeFocus();
      return true;
    }
    if (event == Event::CtrlQ) {
      m_screen.Exit();
      return true;
    }
    return false;
  });

  auto main_renderer =
      Renderer(event_handler, [this, tab_header, tab_container] {
        return vbox({
            window(hbox({tab_header->Render(), filler()}),
                   tab_container->Render() | flex) |
                flex,
            MakeInputSection(),
        });
      });

  m_screen.Loop(main_renderer);
}
