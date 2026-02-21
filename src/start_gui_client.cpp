#include <guiHelper.h>
#include <imgui.h>

#include <iostream>
#include <memory>

#include "Client.h"


// class

std::vector<UserMessage> messageHistory = {};

void GuiFunc() {
  ImGui::BeginChild("History", ImVec2(0, -80), true);
  for (const auto& msg : messageHistory) {
    ImGui::TextWrapped("%s : %s", msg.m_name.c_str(), msg.m_msg.c_str());
  }
  ImGui::EndChild();
}

static void HandleMessage(std::shared_ptr<Message> msg) {
  if (auto userMsg = std::dynamic_pointer_cast<UserMessage>(msg)) {
    messageHistory.push_back(*userMsg.get());
  }
  else if (auto usersList = std::dynamic_pointer_cast<UsersList>(msg)) {
    std::cout << "[gui] : Users list" << std::endl;
  }
  else if (auto channelsList = std::dynamic_pointer_cast<ChannelsList>(msg)) {
    std::cout << "[gui] : Channels list" << std::endl;
  }
  else {
    std::cout << "[gui] : Unknown message type" << std::endl;
  }
}

static void ClentDoWork() {
  try {
    Client client{};

    client.SetupMessageHandler(&HandleMessage);

    std::cout << "Connecting to localhost...\n";
    client.connect("127.0.0.1", "6456");

    std::cout << "Connected. Commands: /nick [name], /join [channel]\n";
    client.run();
  }
  catch (const std::exception& e) {
    std::cerr << "Client Thread Error: " << e.what() << '\n';
  }
}

int main() {
  try {
    GuiHelper::Window window("Client", 800, 600);

    window.SetupGuiFunc(GuiFunc);

    // TODO API interface for working with the engine.
    // TODO implementation of a window for the client part.

    std::thread overflowThread(&ClentDoWork);

    window.RenderLoop();

    overflowThread.join();
  }
  catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << '\n';
  }
}