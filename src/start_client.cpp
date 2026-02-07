#include <iostream>

#include "Client.h"

int main() {
  try {
    Client client{};
    std::cout << "Connecting to localhost...\n";
    client.connect("127.0.0.1", "8080");

    std::cout << "Connected. Commands: /nick [name], /join [channel]\n";
    client.run();
  }
  catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << '\n';
  }
}
