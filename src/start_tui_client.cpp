#include <iostream>

#include "TuiApp.h"

int main() {
  try {
    TuiApp app;
    app.run();
  }
  catch (std::exception& e) {
    std::cerr << "Error: " << e.what() << '\n';
  }
}
