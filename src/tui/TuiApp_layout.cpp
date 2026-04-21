/*
  Main layout, workspace lists, create-channel modal, and app bootstrap.
  This file wires the screen together and handles global keys such as tab
  switching, modal dismissal, and quitting the application.
*/
#include <algorithm>
#include <cctype>

#include "TuiApp.h"

Component TuiApp::make_section_view(std::string title) {
  return Renderer([title = std::move(title)] {
    return vbox({
        text(title),
        separator(),
    });
  });
}

Component TuiApp::make_tab_header() {
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

Component TuiApp::make_workspace(std::string menu_title,
                                 WorkspaceState& state) {
  if (!state.items.empty()) {
    if (state.selected_item >= static_cast<int>(state.items.size())) {
      state.selected_item = static_cast<int>(state.items.size()) - 1;
    }
  }
  else {
    state.selected_item = 0;
  }

  std::vector<std::string>* display_items = nullptr;
  if (&state == &m_dms_state) {
    display_items = &m_dms_display_items;
  }
  else if (&state == &m_channels_state) {
    display_items = &m_channels_display_items;
  }
  else {
    display_items = &state.items;
  }

  MenuOption option;
  option.on_enter = [this, &state] {
    if (&state == &m_channels_state && state.selected_item >= 0 &&
        state.selected_item < static_cast<int>(state.items.size())) {
      m_client.send_message("/join " + state.items[state.selected_item]);
    }

    state.active_item = state.selected_item;
    state.mode = to_index(ViewMode::Chat);
    m_in_chat = true;

    const std::string active_name = get_active_item_name(state);
    const bool channels = (&state == &m_channels_state);

    auto& scroll = get_scroll_state(channels, active_name);
    scroll.follow_bottom = true;

    const auto& history = channels ? m_channel_histories[active_name]
                                   : m_dm_histories[active_name];
    scroll.selected_line =
        history.empty() ? -1 : static_cast<int>(history.size()) - 1;

    m_input_field->TakeFocus();
  };

  auto menu = Menu(display_items, &state.selected_item, option);

  auto list_view = Renderer(menu, [menu, menu_title] {
    return vbox({
        text(" " + menu_title + " ") | center,
        separator(),
        menu->Render(),
        filler(),
        text("[ Press ENTER to join ]") | center,
    });
  });

  auto chat_view = make_chat_view(state);
  return Container::Tab({list_view, chat_view}, &state.mode);
}

Component TuiApp::make_create_channel_modal() {
  auto on_create = [this] {
    if (!m_new_channel_input.empty()) {
      m_client.send_message("/create " + m_new_channel_input);
      m_new_channel_input.clear();
      m_modal_layer = 0;
    }
  };

  auto specialized_input =
      CatchEvent(m_modal_input_field, [this, on_create](Event event) {
        if (event == Event::Return) {
          on_create();
          return true;
        }

        if (event.is_character()) {
          char c = event.character()[0];
          if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
            return true;
          }
        }
        return false;
      });

  auto buttons = Container::Horizontal({
                     Button("Create", on_create),
                     Button("Cancel",
                            [this] {
                              m_new_channel_input.clear();
                              m_modal_layer = 0;
                            }),
                 }) |
                 center;

  auto component = Container::Vertical({
      specialized_input,
      buttons,
  });

  return Renderer(component, [component] {
    return vbox({
               text(" Create New Channel ") | bold | center,
               separator(),
               component->Render(),
               filler(),
               text("3-20 alphanumeric characters") | dim | center,
           }) |
           border | size(WIDTH, EQUAL, 40) | size(HEIGHT, EQUAL, 10) | center;
  });
}

Element TuiApp::make_input_section() {
  if (!m_in_chat) {
    return text("");
  }
  return window(text(""), m_input_field->Render()) | size(HEIGHT, EQUAL, 3);
}

Component TuiApp::make_main_view() {
  auto tab_header = make_tab_header();
  auto dms_workspace = make_workspace("AVAILABLE DMS", m_dms_state);
  auto channels_workspace =
      make_workspace("AVAILABLE CHANNELS", m_channels_state);
  // auto settings_view = make_section_view(" SETTINGS ");

  auto tab_container =
      Container::Tab({dms_workspace, channels_workspace}, &m_tab_selected);

  auto main_container = Container::Vertical({
      tab_header,
      tab_container,
      m_input_field,
  });

  auto modal_container = make_create_channel_modal();
  auto root_container =
      Container::Tab({main_container, modal_container}, &m_modal_layer);

  auto event_handler = CatchEvent(
      root_container, [this, tab_header, tab_container](Event event) {
        if (m_stage == to_index(AppStage::Auth)) {
          return false;
        }

        auto exit_chat_mode = [this, tab_container] {
          if (!m_in_chat) {
            return;
          }

          m_in_chat = false;

          if (m_tab_selected == to_index(TabEntry::DMs)) {
            m_dms_state.mode = to_index(ViewMode::List);
          }
          else if (m_tab_selected == to_index(TabEntry::Channels)) {
            m_channels_state.mode = to_index(ViewMode::List);
          }

          tab_container->TakeFocus();
        };

        if (event == Event::Escape) {
          if (m_modal_layer == 1) {
            m_modal_layer = 0;
            return true;
          }

          if (m_in_chat) {
            m_in_chat = false;
            if (m_tab_selected == to_index(TabEntry::DMs)) {
              m_dms_state.mode = to_index(ViewMode::List);
            }
            else if (m_tab_selected == to_index(TabEntry::Channels)) {
              m_channels_state.mode = to_index(ViewMode::List);
            }
            tab_container->TakeFocus();
            return true;
          }
        }
        if (event == Event::Character('n') &&
            m_tab_selected == to_index(TabEntry::Channels) &&
            m_channels_state.mode == to_index(ViewMode::List) &&
            m_modal_layer == 0) {
          m_modal_layer = 1;
          return true;
        }
        if (event == Event::F1) {
          exit_chat_mode();
          m_tab_selected = to_index(TabEntry::DMs);
          tab_header->TakeFocus();
          return true;
        }
        if (event == Event::F2) {
          exit_chat_mode();
          m_tab_selected = to_index(TabEntry::Channels);
          m_client.send_message("/channels");
          tab_header->TakeFocus();
          return true;
        }
        // TODO: Add settings
        /*if (event == Event::F12) {
          m_tab_selected = to_index(TabEntry::Settings);
          tab_header->TakeFocus();
          return true;
        }*/
        if (event == Event::CtrlQ) {
          m_screen.Exit();
          return true;
        }

        return false;
      });

  return Renderer(
      event_handler, [this, tab_header, tab_container, modal_container] {
        auto base_ui = vbox({
            window(hbox({tab_header->Render(), filler()}),
                   tab_container->Render() | flex) |
                flex,
            make_input_section(),
        });

        if (m_modal_layer == 1) {
          return dbox({base_ui, modal_container->Render() | center});
        }

        return base_ui;
      });
}

void TuiApp::run(const std::string& ip, const std::string& port) {
  m_client.register_message_callback([this](std::shared_ptr<Message> msg) {
    handle_incoming_message(std::move(msg));
    m_screen.PostEvent(Event::Custom);
  });

  m_client.connect(ip, port);

  auto auth_view = make_auth_view();
  auto main_view = make_main_view();

  auto root_container = Container::Tab({auth_view, main_view}, &m_stage);

  auto app_renderer = Renderer(root_container, [this, auth_view, main_view] {
    if (m_stage == to_index(AppStage::Auth)) {
      return auth_view->Render();
    }
    return main_view->Render();
  });

  m_screen.Loop(app_renderer);
}
