#include <iostream>

#include "Server.h"

int main() {
  try {
    std::cout << "Creating a server...\n";
    Server server{"127.0.0.1", "8080"};
    server.run();
  }
  catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << '\n';
  }
}
