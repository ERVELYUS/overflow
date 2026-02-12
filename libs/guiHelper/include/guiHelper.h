#pragma once

#include <string_view>

class GLFWwindow;

namespace GuiHelper {

class Window {
 public:
  Window(std::string_view title, int width, int height);
  ~Window();
  void RenderLoop();

 private:
  GLFWwindow* m_windowPtr = nullptr;

  bool CreateWindow(std::string_view title, int width, int height);
  void CleanUp();
};

}  // namespace GuiHelper
