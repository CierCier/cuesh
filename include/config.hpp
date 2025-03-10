#pragma once

#include <string>

class Config {
public:
  std::string cwd;
  int errorno = 0;

  Config(const std::string &cwd) { this->cwd = std::move(cwd); }
};