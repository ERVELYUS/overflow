#include <iostream>
#include <string>

#include "TuiApp.h"

int main(int argc, char** argv) {
  std::string ip = "127.0.0.1";
  std::string port = "8080";

  if (argc == 2) {
    port = argv[1];
  }
  else if (argc == 3) {
    ip = argv[1];
    port = argv[2];
  }
  else {
    std::cerr << "Usage: " << argv[0] << " [ip] [port]\n";
    return 1;
  }

  try {
    TuiApp app;
    app.run(ip, port);
  }
  catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << '\n';
  }
}
