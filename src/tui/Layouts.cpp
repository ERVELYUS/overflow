#include "Layouts.h"

using namespace ftxui;

Element TuiDesign::RenderMainLayout(
    const std::vector<std::string>& chat_history,
    const std::vector<std::string>& user_list,
    Element input_field) {  // <--- Add this parameter

  Elements chat_lines;
  for (const auto& line : chat_history) chat_lines.push_back(text(line));
  auto chat_view = vbox(std::move(chat_lines)) | frame | flex | border;

  Elements user_lines;
  for (const auto& user : user_list)
    user_lines.push_back(text(" ● " + user) | color(Color::Green));
  auto sidebar = vbox(std::move(user_lines)) | size(WIDTH, EQUAL, 20) | border;

  return vbox({
      hbox({chat_view | flex, sidebar}) |
          flex,  // The chat and sidebar take up all available top space
      hbox({text(" > "), input_field | flex}) |
          border  // The input bar sits at the very bottom
  });
}
