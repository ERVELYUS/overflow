#include <GLFW/glfw3.h>
#include <guiHelper.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

GuiHelper::Window::Window(std::string_view title, int width, int height) {
  CreateWindow(title, width, height);
}

GuiHelper::Window::~Window() { CleanUp(); }

constexpr int minWidth = 200;
constexpr int minHeight = 200;

static void windowSizeCallback(GLFWwindow* window, int width, int height) {
  width = std::max(width, minWidth);
  height = std::max(height, minHeight);
  glfwSetWindowSize(window, width, height);

  GuiHelper::Window* instance =
      static_cast<GuiHelper::Window*>(glfwGetWindowUserPointer(window));

  if (instance) {
    instance->RenderFrame();
  }
}
void GuiHelper::Window::SetupGuiFunc(std::function<void()> func) {
  m_renderFunc = func;
}

void GuiHelper::Window::RenderFrame() {
  glfwGetWindowSize(m_windowPtr, &m_windowWidth, &m_windowHeight);
  // New frame ImGui.
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  // GUI code.

  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImVec2(m_windowWidth, m_windowHeight));
  // Флаги для окна без рамки и заголовка
  /* ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

  ImGui::Begin("Chat", nullptr, window_flags);

  m_renderFunc();

  ImGui::End(); */
  ImGui::ShowDemoWindow();

  // Rendering.

  ImGui::Render();
  glViewport(0, 0, m_windowWidth, m_windowHeight);
  glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
  glClear(GL_COLOR_BUFFER_BIT);
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  glfwSwapBuffers(m_windowPtr);
}
void GuiHelper::Window::RenderLoop() {
  while (!glfwWindowShouldClose(m_windowPtr)) {
    glfwPollEvents();
    RenderFrame();
  }
}

bool GuiHelper::Window::CreateWindow(std::string_view title, int width,
                                     int height) {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  m_windowPtr = glfwCreateWindow(width, height, title.data(), NULL, NULL);
  glfwMakeContextCurrent(m_windowPtr);
  glfwSetWindowUserPointer(m_windowPtr, this);
  glfwSwapInterval(1);  // Vert. Sync.
  glfwSetWindowSizeCallback(m_windowPtr, windowSizeCallback);
  // ImGui init.
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;

  // OpenGL init
  ImGui_ImplGlfw_InitForOpenGL(m_windowPtr, true);
  ImGui_ImplOpenGL3_Init("#version 130");
  return true;
}

void GuiHelper::Window::CleanUp() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(m_windowPtr);
  glfwTerminate();
}
