#include "Layouts.h"

using namespace ftxui;

Element TuiDesign::RenderMainLayout(
    Element header_toggle, const std::vector<std::string>& chat_history,
    const std::vector<std::string>& users_list, Element input_field) {
  // Chat collumn
  Elements chat_lines;
  for (const auto& line : chat_history) {
    chat_lines.push_back(text(line));
  }
  auto chat_history_column =
      vbox({filler(), vbox(std::move(chat_lines))}) | flex | border;

  // Users list
  Elements users_lines;
  for (const auto& name : users_list) {
    users_lines.push_back(text(name));
  }
  auto users_column = vbox({text("Users") | bold | center, separator(),
                            vbox(std::move(users_lines)), filler()}) |
                      border | size(WIDTH, EQUAL, 22);

  return vbox({hbox({header_toggle}) | border,

               hbox({chat_history_column, users_column}) | flex,

               hbox({input_field | flex}) | border | size(HEIGHT, EQUAL, 3)});
}
