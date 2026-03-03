#include "Layouts.h"

using namespace ftxui;

Element TuiDesign::RenderMainLayout(
    Element header_toggle, Element settings_btn,
    const std::vector<std::string>& chat_history, Element input_field) {
  Elements chat_lines;
  for (const auto& line : chat_history) {
    chat_lines.push_back(text(line));
  }

  return vbox(
      {hbox({header_toggle | bold, separator(), filler(), settings_btn}) |
           border,

       vbox({vbox(std::move(chat_lines)) | frame}) | border | flex,

       hbox({input_field | flex}) | border | size(HEIGHT, EQUAL, 3)});
}
