#include <iostream>

#include "Client.h"

int main(int argc, char** argv) {
  // Unlike starting a server, starting a user requires both ip and port
  // It doesn't make sense to provide default ip, because you are trying to
  // connect to someone
  if (argc != 3) {  // User provided incorrect arguments
    std::cerr << "Usage: " << argv[0] << " [ip] [port]\n";
    return 1;
  }

  std::string ip = argv[1];
  std::string port = argv[2];

  try {
    std::cout << "Connecting to " << ip << ':' << port << "...\n";
    Client client{};
    client.connect(ip, port);

    std::cout << "Connected. Commands: /nick [name], /join [channel]\n";
    client.run();
  }
  catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << '\n';
  }
}
