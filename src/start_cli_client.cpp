#include <iostream>

#include "CliApp.h"

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " [ip] [port]\n";
    return 1;
  }

  CliApp app;
  app.run(argv[1], argv[2]);

  return 0;
}
