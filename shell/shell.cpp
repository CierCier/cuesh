#include "shell.hpp"

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>

Shell::Shell() : error(0) {
  // Setup default environment variables
  const char *username = getenv("USER");
  setEnv("USER", username ? username : "unknown");

  // Load environment variables from the system
  char **env = environ;
  while (*env) {
    std::string envVar(*env);
    size_t pos = envVar.find('=');
    if (pos != std::string::npos) {
      std::string key = envVar.substr(0, pos);
      std::string value = envVar.substr(pos + 1);
      setEnv(key, value);
    }
    env++;
  }

  char *user = getenv("USER");
  setEnv("USER", std::string(user ? user : "unknown"));

  // Setup the default prompt
  setEnv("PS1", "$USER@$PWD > ");

  // Setup built-in commands
  CommandMap["cd"] = [this](const std::string &arg) { return cd(arg); };
  CommandMap["pwd"] = [this](const std::string &arg) {
    (void)arg;
    return pwd();
  };

  CommandMap["echo"] = [this](const std::string &arg) { return echo(arg); };
  CommandMap["exit"] = [this](const std::string &arg) { return exit(arg); };
  CommandMap["set"] = [this](const std::string &arg) { return set(arg); };
  CommandMap["unset"] = [this](const std::string &arg) { return unset(arg); };
  CommandMap["history"] = [this](const std::string &arg) {
    (void)arg;
    showCommandHistory();
    return "";
  };
}

Shell::~Shell() { termiosExitRawMode(); }

void Shell::setEnv(const std::string &key, const std::string &value) {
  Environment[key] = value;
}

std::string Shell::getEnv(const std::string &key) {
  auto it = Environment.find(key);
  return it != Environment.end() ? it->second : "";
}

char **Shell::getEnviron() {
  char **env = new char *[Environment.size() + 1];
  size_t i = 0;
  for (const auto &entry : Environment) {
    std::string envVar = entry.first + "=" + entry.second;
    env[i] = new char[envVar.size() + 1];
    std::strcpy(env[i], envVar.c_str());
    i++;
  }
  env[i] = nullptr;
  return env;
}

std::string Shell::replaceEnvVars(const std::string &input) {
  std::string output = input;
  size_t pos = 0;
  while ((pos = output.find('$', pos)) != std::string::npos) {
    size_t end = pos + 1;
    while (end < output.size() &&
           (isalnum(output[end]) || output[end] == '_')) {
      ++end;
    }
    std::string key = output.substr(pos + 1, end - pos - 1);
    std::string value = getEnv(key);
    output.replace(pos, end - pos, value);
    pos += value.size();
  }
  return output;
}

void Shell::printPrompt() {
  std::string prompt = getEnv("PS1");
  std::cout << replaceEnvVars(prompt) << std::flush;
}

void Shell::termiosEnterRawMode() {
  if (tcgetattr(STDIN_FILENO, &oldTermios) == -1) {
    throw std::runtime_error("Failed to get terminal attributes");
  }
  newTermios = oldTermios;
  newTermios.c_lflag &= ~(ICANON | ECHO);
  if (tcsetattr(STDIN_FILENO, TCSANOW, &newTermios) == -1) {
    throw std::runtime_error("Failed to set terminal attributes");
  }
}

void Shell::termiosExitRawMode() {
  tcsetattr(STDIN_FILENO, TCSANOW, &oldTermios);
}

void Shell::start() {
  try {
    termiosEnterRawMode();
    printPrompt();

    while (true) {
      char c;
      if (read(STDIN_FILENO, &c, 1) <= 0) {
        break; // Exit on EOF or error
      }

      switch (c) {
      case '\n':
        std::cout << std::endl;
        processInput(); // Process the input
        Buffer.clear();
        printPrompt();
        break;
      case 127: // Backspace
      case 8:   // Ctrl-H
        if (!Buffer.empty()) {
          Buffer.pop_back();
          std::cout << "\b \b" << std::flush;
        }
        break;
      default:
        Buffer += c;
        std::cout << c << std::flush;
        break;
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
  }

  termiosExitRawMode();
}
void Shell::processInput() {
  std::string command = Buffer;
  if (command.empty()) {
    return;
  }

  size_t spacePos = command.find(' ');
  std::string cmd =
      (spacePos == std::string::npos) ? command : command.substr(0, spacePos);
  std::string arg =
      (spacePos == std::string::npos) ? "" : command.substr(spacePos + 1);

  auto it = CommandMap.find(cmd);
  if (it != CommandMap.end()) {
    std::cout << it->second(arg) << std::endl;
  } else {
    executeCommand(command);
  }

  addToCommandHistory(command);
}

int Shell::executeCommand(const std::string &command) {
  int pid = fork();
  if (pid == 0) {
    // Child process
    char **env = getEnviron();
    size_t pos = command.find(' ');

    // If there is a space, we need to split the command and arguments
    if (pos != std::string::npos) {
      std::string cmd = command.substr(0, pos);
      std::string arg_string = command.substr(pos + 1);

      // split the args
      std::vector<std::string> args;
      size_t start = 0;
      while (start < arg_string.size()) {
        size_t end = arg_string.find(' ', start);
        if (end == std::string::npos) {
          end = arg_string.size();
        }
        args.push_back(arg_string.substr(start, end - start));
        start = end + 1;
      }

      // convert to char**
      char **args_array = new char *[args.size() + 2];
      args_array[0] = strdup(cmd.c_str());
      for (size_t i = 0; i < args.size(); i++) {
        args_array[i + 1] = strdup(args[i].c_str());
      }
      args_array[args.size() + 1] = nullptr;

      execvpe(cmd.c_str(), args_array, env);
    } else {
      char *args[] = {strdup(command.c_str()), nullptr};
      execvpe(command.c_str(), args, env);
    }

    // If execve returns, it failed
    std::cerr << "Failed to execute command: " << command << std::endl;
    ::exit(1);

  } else if (pid < 0) {
    std::cerr << "Failed to fork process" << std::endl;
    return 1;
  } else {
    // Parent process
    int status;
    waitpid(pid, &status, 0);
    return status;
  }
}

std::string Shell::cd(const std::string &path) {
  if (chdir(path.c_str()) == 0) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd))) {
      setEnv("PWD", cwd);
      return "";
    }
  }
  return "cd: " + path + ": No such file or directory";
}

std::string Shell::pwd() {
  char cwd[1024];
  if (getcwd(cwd, sizeof(cwd))) {
    return cwd;
  }
  return "pwd: error retrieving current directory";
}

std::string Shell::echo(const std::string &message) {
  std::string output = replaceEnvVars(message);
  return output;
}

std::string Shell::exit(const std::string &code) {
  int exitCode = 0;
  if (!code.empty()) {
    try {
      exitCode = std::stoi(code);
    } catch (const std::exception &e) {
      std::cerr << "exit: " << e.what() << std::endl;
    }
  }

  ::exit(exitCode);
}

std::string Shell::set(const std::string &assignment) {
  size_t pos = assignment.find(' ');
  if (pos != std::string::npos) {
    std::string key = assignment.substr(0, pos);
    std::string value = assignment.substr(pos + 1);
    value = replaceEnvVars(value);
    // figure out if we have a math expression

    setEnv(key, value);
  }
  return "";
}

std::string Shell::unset(const std::string &key) {
  Environment.erase(key);
  return "";
}

void Shell::addToCommandHistory(const std::string &command) {
  CommandHistory.push_back(command);
  CommandHistoryIndex = CommandHistory.size();
}

void Shell::showCommandHistory() {
  for (size_t i = 0; i < CommandHistory.size(); i++) {
    std::cout << i << ": " << CommandHistory[i] << std::endl;
  }
}