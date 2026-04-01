#include "TuiApp.h"

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

TuiApp::TuiApp() : m_running(true), m_screen(ScreenInteractive::Fullscreen()) {
  m_channels_state.items = {"#general", "#dev", "#random"};
  m_dms_state.items = {"@John_Doe", "@Jane_Doe", "@Pablo"};

  auto input_base = Input(&m_input_buffer, "type here...");
  m_input_field = CatchEvent(input_base, [this](Event event) {
    if (event == Event::Return && !m_input_buffer.empty()) {
      m_dummy_msgs.push_back(m_nickname + ": " + m_input_buffer);
      m_input_buffer.clear();
      return true;
    }
    return false;
  });
};

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
Component TuiApp::MakeChatView(const WorkspaceState& state) {
  return Renderer([this, &state] {
    std::vector<Element> lines;
    for (auto const& msg : m_dummy_msgs) {
      size_t colon_pos = msg.find(":");

      if (colon_pos != std::string::npos) {
        std::string nickname = msg.substr(0, colon_pos);
        Element colored_nickname =
            text(nickname) |
            color(nickname == m_nickname ? Color::Red : Color::Blue);
        Element message = text(msg.substr(colon_pos));
        lines.push_back(hbox({colored_nickname, message}));
      }
      else {
        lines.push_back(text(msg) | dim);
      }
    }

    std::string display_title = "";
    if (state.active_item >= 0 && state.active_item < state.items.size()) {
      display_title = state.items[state.active_item];
    }

    return vbox({text(" " + display_title + " ") | bold | center, separator(),
                 filler(), vbox(std::move(lines)) | vscroll_indicator}) |
           flex;
  });
}

Component TuiApp::MakeWorkspace(std::string menu_title, WorkspaceState& state) {
  // What to do when chat is selected
  MenuOption option;
  option.on_enter = [this, &state] {
    state.active_item = state.selected_item;
    state.mode = to_index(ViewMode::Chat);
    m_in_chat = true;
    m_input_field->TakeFocus();
  };

  auto menu = Menu(&state.items, &state.selected_item, option);
  auto list_view = Renderer(menu, [menu, menu_title] {
    return vbox({
        text(" " + menu_title + " ") | center,
        separator(),
        menu->Render(),
        filler(),
        text("[ Press ENTER to join ]") | center,
    });
  });

  auto chat_view = MakeChatView(state);

  auto router = Container::Tab({list_view, chat_view}, &state.mode);

  return CatchEvent(router, [this, menu, &state](Event event) {
    if (state.mode == to_index(ViewMode::Chat) && event == Event::Escape) {
      state.mode = to_index(ViewMode::List);
      m_in_chat = false;
      menu->TakeFocus();
      return true;
    }
    return false;
  });
};

// How does input field look and when it's shown
Element TuiApp::MakeInputSection() {
  if (!m_in_chat) {
    return text("");
  }
  return window(text(""), m_input_field->Render()) | size(HEIGHT, EQUAL, 3);
}

void TuiApp::run() {
  auto tab_header = MakeTabHeader();
  auto dms_workspace = MakeWorkspace("AVAILABLE DMS", m_dms_state);
  auto channels_workspace =
      MakeWorkspace("AVAILABLE CHANNELS", m_channels_state);
  auto settings_view = MakeSectionView(" SETTINGS ");

  auto tab_container = Container::Tab(
      {dms_workspace, channels_workspace, settings_view}, &m_tab_selected);

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
