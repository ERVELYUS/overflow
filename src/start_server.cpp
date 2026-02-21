#include <iostream>

#include "Server.h"

int main(int argc, char** argv) {
  // Default values for server
  std::string ip = "0.0.0.0";
  std::string port = "8080";

  if (argc == 2) {  // User provided only a port, starting at 0.0.0.0
    port = argv[1];
  }
  else if (argc == 3) {  // User provided both ip and a port
    ip = argv[1];
    port = argv[2];
  }
  else {  // User provided incorrect arguments
    std::cerr << "Usage: " << argv[0] << " [ip] [port] / " << argv[0]
              << " [port]\n";
    return 1;
  }

  try {
    std::cout << "Creating a server on " << ip << ':' << port << "...\n";
    Server server{ip, port};
    server.run();
  }
  catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << '\n';
  }
}
