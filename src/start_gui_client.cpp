#include <iostream>

#include <guiHelper.h>
#include "Client.h"

int main() {
  try {

    GuiHelper::Window window("Client", 800, 600);

    window.RenderLoop();

    // TODO API interface for working with the engine.
    // TODO implementation of a window for the client part.

    // Client client{};S
    // std::cout << "Connecting to localhost...\n";
    // client.connect("127.0.0.1", "8080");

    // std::cout << "Connected. Commands: /nick [name], /join [channel]\n";
    // client.run();
  }
  catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << '\n';
  }
}