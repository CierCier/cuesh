#pragma once

#include "tokenizer.hpp"
#include <functional>
#include <string>
#include <termios.h>
#include <unordered_map>
#include <vector>

class Shell {
public:
  using CommandFunction = std::function<int(const std::vector<Token> &)>;

  Shell();

  // Core execution functions
  int init();
  int run();
  int exitShell();

  int runCommand(const std::vector<Token> &tokens);

private:
  std::unordered_map<std::string, CommandFunction> commandMap;

  // Built-in command handlers
  int cd(const std::vector<Token> &tokens);
  int quit(const std::vector<Token> &tokens);
  int clear(const std::vector<Token> &tokens);

  // Executes external commands found in $PATH
  int executeExternalCommand(const std::vector<Token> &tokens);

  // Shell environment variables
  std::unordered_map<std::string, std::string> env;
  // Environment variables from the system
  char **cEnviron;

  // Helper functions

  // this also updates environ for execvpe
  void setEnv(const std::string &key, const std::string &value);
  std::string getEnv(const std::string &key);

  std::string prompt;
  void printPrompt();

  // terminal mode
  struct termios originalTermios;
  void enableRawMode();
  void disableRawMode();
  void handleSignal(char c);

  // shell buffers
  std::string inputBuffer;
  std::vector<Token> tokens;

  // tokenization
  Tokenizer tokenizer;
  // replace env variables in tokens in place
  void replaceVariables(std::vector<Token> &tokens);
  std::string replaceVariable(const std::string &value);
};
