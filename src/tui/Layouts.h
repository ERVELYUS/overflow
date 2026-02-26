#pragma once
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

namespace TuiDesign {
ftxui::Element RenderMainLayout(const std::vector<std::string>& chat_history,
                                const std::vector<std::string>& user_list,
                                ftxui::Element input_field);
}
