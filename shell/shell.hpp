#pragma once

#include "language/tokenizer.hpp"
#include <functional>
#include <string>
#include <termios.h>
#include <unordered_map>
#include <vector>

class Shell {
public:
  Shell();
  ~Shell();

  void start();

  int getError() { return error; }

private:
  Tokenizer tokenizer;

  int error; // internally used to pass error codes;

  std::string Buffer;

  void processInput();
  int executeCommand(const std::string &command,
                     const std::vector<std::string> &args);

  // Environment
  std::unordered_map<std::string, std::string> Environment;
  void setEnv(const std::string &key, const std::string &value);
  std::string getEnv(const std::string &key);
  char **getEnviron();

  std::string replaceEnvVars(const std::string &input);

  void printPrompt();

  // Termios
  struct termios oldTermios;
  struct termios newTermios;
  void termiosEnterRawMode();
  void termiosExitRawMode();

  // builtins
  std::string cd(const std::string &path);
  std::string pwd();
  std::string echo(const std::string &message);
  void exit(const int &code);
  void set(const std::string &key,
           const std::string &value);          // set an environment variable
  void unset(const std::string &key);          // unset an environment variable
  std::string history(const std::string &arg); // show command history

  // command map
  std::unordered_map<
      std::string, std::function<std::string(const std::vector<std::string> &)>>
      CommandMap;

  // command history
  std::vector<std::string> CommandHistory;
  size_t CommandHistoryIndex;
  void addToCommandHistory(const std::string &command);
  void showCommandHistory();
};