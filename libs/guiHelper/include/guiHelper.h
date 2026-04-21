#pragma once

#include <functional>
#include <string_view>

struct GLFWwindow;

namespace GuiHelper {

class Window {
 public:
  Window(std::string_view title, int width, int height);
  ~Window();
  void SetupGuiFunc(std::function<void()> func);
  void RenderFrame();
  void RenderLoop();

 private:
  GLFWwindow* m_windowPtr = nullptr;
  std::function<void()> m_renderFunc;
  int m_windowWidth;
  int m_windowHeight;

  bool CreateWindow(std::string_view title, int width, int height);
  void CleanUp();
};

}  // namespace GuiHelper
