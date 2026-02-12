cmake_minimum_required(VERSION 3.10)
project(guiHelper VERSION 1.0)

include(FetchContent)

set(OpenGL_GL_PREFERENCE "GLVND")

find_package(OpenGL REQUIRED)

# GLFW
FetchContent_Declare(glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
)

FetchContent_MakeAvailable(glfw)

set(HELPER_SOURCES
  guiHelper.cpp
  3rdparty/imgui/imgui.cpp
  3rdparty/imgui/imgui_impl_glfw.cpp
  3rdparty/imgui/imgui_impl_opengl3.cpp
  3rdparty/imgui/imgui_demo.cpp
  3rdparty/imgui/imgui_draw.cpp
  3rdparty/imgui/imgui_tables.cpp
  3rdparty/imgui/imgui_widgets.cpp
)

add_library(guiHelper STATIC ${HELPER_SOURCES})

target_include_directories(guiHelper PRIVATE include)
target_include_directories(guiHelper PRIVATE 3rdparty/imgui)

target_link_libraries(guiHelper glfw OpenGL::GL)