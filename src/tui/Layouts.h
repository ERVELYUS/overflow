#pragma once
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

namespace TuiDesign {
ftxui::Element RenderMainLayout(ftxui::Element header_toggle,
                                ftxui::Element settings_btn,
                                const std::vector<std::string>& chat_history,
                                ftxui::Element input_field);
}
