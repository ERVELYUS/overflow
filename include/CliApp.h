#pragma once

#include <string>

#include "Client.h"

class CliApp {
 public:
  CliApp();
  void run(const std::string& ip, const std::string& port);

 private:
  Client m_client;

  void setup_callbacks();
};
