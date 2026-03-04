#pragma once
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

namespace TuiDesign {
ftxui::Element RenderMainLayout(ftxui::Element header_toggle,
                                const std::vector<std::string>& chat_history,
                                const std::vector<std::string>& users_list,
                                ftxui::Element input_field);
}
