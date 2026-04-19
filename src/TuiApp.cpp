#include "TuiApp.h"

#include <algorithm>
#include <cctype>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "Message.h"

TuiApp::TuiApp() : m_running(true), m_screen(ScreenInteractive::Fullscreen()) {
  m_channels_state.items.clear();
  m_dms_state.items.clear();

  m_modal_input_field = Input(&m_new_channel_input, "New channel name...");

  // Main chat input
  auto input_base = Input(&m_input_buffer, "type here...");
  m_input_field = CatchEvent(input_base, [this](Event event) {
    if (event == Event::Return && !m_input_buffer.empty()) {
      m_client.send_message(m_input_buffer);

      if (m_tab_selected == to_index(TabEntry::Channels)) {
        const std::string key = GetActiveItemName(m_channels_state);
        if (!key.empty()) {
          AppendLine(m_channel_histories[key], m_nickname, m_input_buffer);
        }
      }
      else if (m_tab_selected == to_index(TabEntry::DMs)) {
        const std::string key = GetActiveItemName(m_dms_state);
        if (!key.empty()) {
          m_client.send_message("/pm " + key + " " + m_input_buffer);
          AppendLine(m_dm_histories[key], m_nickname, m_input_buffer);
        }
      }

      m_input_buffer.clear();
      return true;
    }
    return false;
  });

  // Auth inputs
  m_auth_username_field = Input(&m_auth_username, "nickname...");
  m_auth_password_field = Input(&m_auth_password, "password...");
}

void TuiApp::AppendLine(std::vector<ChatLine>& history,
                        const std::string& sender, const std::string& text,
                        bool system) {
  history.push_back(ChatLine{sender, text, system});
}

void TuiApp::SubmitAuth(bool register_flow) {
  if (m_auth_username.empty() || m_auth_password.empty()) {
    m_auth_status = "Please enter both nickname and password.";
    return;
  }

  m_auth_status = register_flow ? "Registering..." : "Logging in...";
  const std::string command = (register_flow ? "/register " : "/login ") +
                              m_auth_username + " " + m_auth_password;

  m_client.send_message(command);
}

void TuiApp::HandleIncomingMessage(std::shared_ptr<Message> msg) {
  if (auto sysMsg = std::dynamic_pointer_cast<SystemMessage>(msg)) {
    if (m_stage == to_index(AppStage::Auth)) {
      m_auth_status = sysMsg->m_text;
    }
    else {
      const std::string target = GetActiveItemName(
          m_tab_selected == to_index(TabEntry::Channels) ? m_channels_state
                                                         : m_dms_state);

      if (!target.empty()) {
        auto& history = (m_tab_selected == to_index(TabEntry::Channels))
                            ? m_channel_histories[target]
                            : m_dm_histories[target];

        AppendLine(history, "System", sysMsg->m_text, true);
      }
    }
  }
  else if (auto userMsg = std::dynamic_pointer_cast<UserMessage>(msg)) {
    // If we're currently in channels, append to the active channel history.
    if (m_tab_selected == to_index(TabEntry::Channels)) {
      const std::string key = GetActiveItemName(m_channels_state);
      if (!key.empty()) {
        AppendLine(m_channel_histories[key], userMsg->m_name, userMsg->m_msg);
      }
    }
    else {
      const std::string key = userMsg->m_name;
      AppendLine(m_dm_histories[key], userMsg->m_name, userMsg->m_msg);
    }
  }
  else if (auto usersList = std::dynamic_pointer_cast<UsersList>(msg)) {
    m_dms_state.items.clear();
    m_dms_display_items.clear();

    for (const auto& user : usersList->m_users) {
      if (user == m_nickname) continue;
      m_dms_state.items.push_back(user);          // plain
      m_dms_display_items.push_back("@" + user);  // UI only
    }
  }
  else if (auto channelsList = std::dynamic_pointer_cast<ChannelsList>(msg)) {
    m_channels_state.items.clear();
    m_channels_display_items.clear();

    for (const auto& chan : channelsList->m_channels) {
      m_channels_state.items.push_back(chan);          // plain
      m_channels_display_items.push_back("#" + chan);  // UI only
    }
  }
  else if (auto selfMsg = std::dynamic_pointer_cast<SelfNameMessage>(msg)) {
    m_nickname = selfMsg->m_name;

    if (m_stage == to_index(AppStage::Auth)) {
      m_stage = to_index(AppStage::Main);
      m_auth_status = "Authenticated as " + m_nickname;
      m_auth_password.clear();
      m_in_chat = false;

      m_client.send_message("/channels");
      m_client.send_message("/users");

      m_input_field->TakeFocus();
    }
  }
  else if (auto joinedMsg =
               std::dynamic_pointer_cast<JoinedChannelMessage>(msg)) {
    const std::string& channel = joinedMsg->m_channel;

    m_stage = to_index(AppStage::Main);
    m_in_chat = true;

    // Make sure the channel exists in the UI history map
    (void)m_channel_histories[channel];

    // Select the joined channel in the Channels tab if present
    m_tab_selected = to_index(TabEntry::Channels);
    auto it = std::find(m_channels_state.items.begin(),
                        m_channels_state.items.end(), channel);
    if (it != m_channels_state.items.end()) {
      m_channels_state.active_item =
          static_cast<int>(std::distance(m_channels_state.items.begin(), it));
    }

    m_auth_status.clear();
  }
}

// Auth screen
Component TuiApp::MakeAuthView() {
  auto username_field = CatchEvent(m_auth_username_field, [this](Event event) {
    if (event == Event::Return) {
      m_auth_password_field->TakeFocus();
      return true;
    }
    return false;
  });

  auto password_field = CatchEvent(m_auth_password_field, [this](Event event) {
    if (event == Event::Return) {
      SubmitAuth(false);
      return true;
    }
    return false;
  });

  auto buttons = Container::Horizontal({
      Button("Login", [this] { SubmitAuth(false); }),
      Button("Register", [this] { SubmitAuth(true); }),
  });

  auto form = Container::Vertical({
      username_field,
      password_field,
      buttons,
  });

  return Renderer(form, [this, form] {
    Element status = m_auth_status.empty()
                         ? text("Use Login or Register to continue.") | dim
                         : text(m_auth_status);

    return vbox({
               text(" Authentication ") | bold | center,
               separator(),
               text("Nickname"),
               form->Render(),
               separator(),
               status,
           }) |
           border | size(WIDTH, EQUAL, 54) | size(HEIGHT, EQUAL, 14) | center;
  });
}

// A dud to later use for DMs and Settings
Component TuiApp::MakeSectionView(std::string title) {
  return Renderer([title = std::move(title)] {
    return vbox({
        text(title),
        separator(),
    });
  });
}

// Tab header
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

// Chat room
Component TuiApp::MakeChatView(const WorkspaceState& state) {
  return Renderer([this, &state] {
    std::vector<Element> lines;

    const std::string active_name = GetActiveItemName(state);
    const std::vector<ChatLine>* history = nullptr;

    if (!active_name.empty()) {
      if (m_tab_selected == to_index(TabEntry::Channels)) {
        auto it = m_channel_histories.find(active_name);
        if (it != m_channel_histories.end()) history = &it->second;
      }
      else if (m_tab_selected == to_index(TabEntry::DMs)) {
        auto it = m_dm_histories.find(active_name);
        if (it != m_dm_histories.end()) history = &it->second;
      }
    }

    if (history != nullptr) {
      for (const auto& line : *history) {
        if (line.system) {
          lines.push_back(text(line.text) | dim);
        }
        else {
          Element sender =
              text(line.sender) |
              color(line.sender == m_nickname ? Color::Red : Color::Blue);
          Element msg = text(": " + line.text);
          lines.push_back(hbox({sender, msg}));
        }
      }
    }

    std::string display_title = active_name;
    if (m_tab_selected == to_index(TabEntry::DMs)) {
      display_title = "@" + active_name;
    }
    else if (m_tab_selected == to_index(TabEntry::Channels)) {
      display_title = "#" + active_name;
    }

    return vbox({
               text(" " + display_title + " ") | bold | center,
               separator(),
               filler(),
               vbox(std::move(lines)) | vscroll_indicator,
           }) |
           flex;
  });
}

Component TuiApp::MakeWorkspace(std::string menu_title, WorkspaceState& state) {
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

  auto chat_view = MakeChatView(state);
  return Container::Tab({list_view, chat_view}, &state.mode);
}

Component TuiApp::MakeCreateChannelModal() {
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

Element TuiApp::MakeInputSection() {
  if (!m_in_chat) {
    return text("");
  }
  return window(text(""), m_input_field->Render()) | size(HEIGHT, EQUAL, 3);
}

Component TuiApp::MakeMainView() {
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

  auto modal_container = MakeCreateChannelModal();
  auto root_container =
      Container::Tab({main_container, modal_container}, &m_modal_layer);

  auto event_handler = CatchEvent(
      root_container, [this, tab_header, tab_container](Event event) {
        if (m_stage == to_index(AppStage::Auth)) {
          return false;
        }

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
          m_tab_selected = to_index(TabEntry::DMs);
          tab_header->TakeFocus();
          return true;
        }
        if (event == Event::F2) {
          m_tab_selected = to_index(TabEntry::Channels);
          m_client.send_message("/channels");
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

  return Renderer(
      event_handler, [this, tab_header, tab_container, modal_container] {
        auto base_ui = vbox({
            window(hbox({tab_header->Render(), filler()}),
                   tab_container->Render() | flex) |
                flex,
            MakeInputSection(),
        });

        if (m_modal_layer == 1) {
          return dbox({base_ui, modal_container->Render() | center});
        }

        return base_ui;
      });
}

void TuiApp::run(const std::string& ip, const std::string& port) {
  m_client.register_message_callback([this](std::shared_ptr<Message> msg) {
    HandleIncomingMessage(std::move(msg));
    m_screen.PostEvent(Event::Custom);
  });

  m_client.connect(ip, port);

  auto auth_view = MakeAuthView();
  auto main_view = MakeMainView();

  auto root_container = Container::Tab({auth_view, main_view}, &m_stage);

  auto renderer = Renderer(root_container, [auth_view, main_view] {
    return auth_view->Render() | center;
  });

  auto app_renderer = Renderer(root_container, [this, auth_view, main_view] {
    if (m_stage == to_index(AppStage::Auth)) {
      return auth_view->Render();
    }
    return main_view->Render();
  });

  m_screen.Loop(app_renderer);
}
