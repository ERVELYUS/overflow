#include <guiHelper.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

GuiHelper::Window::Window(std::string_view title, int width, int height) {
  CreateWindow(title, width, height);
}

GuiHelper::Window::~Window() { CleanUp(); }

void GuiHelper::Window::RenderLoop() {
  while (!glfwWindowShouldClose(m_windowPtr)) {
    glfwPollEvents();

    // New frame ImGui.
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // GUI code.

    ImGui::ShowDemoWindow();

    // Rendering.

    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(m_windowPtr, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(m_windowPtr);
  }
}

bool GuiHelper::Window::CreateWindow(std::string_view title, int width,
                                     int height) {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  m_windowPtr = glfwCreateWindow(width, height, title.data(), NULL, NULL);
  glfwMakeContextCurrent(m_windowPtr);
  glfwSwapInterval(1);  // Vert. Sync.
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
