#include "shell.hpp"
#include "language/tokenizer.hpp"

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

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
  CommandMap["cd"] = [this](const std::vector<std::string> &args) {
    if (args.empty()) {
      return cd(getEnv("HOME"));
    }
    return cd(args[0]);
  };
  CommandMap["pwd"] = [this](const std::vector<std::string> &args) {
    (void)args;
    return pwd();
  };

  CommandMap["echo"] = [this](const std::vector<std::string> &args) {
    std::string arg = "";
    for (const auto &a : args) {
      arg += a + " ";
    }

    return echo(arg);
  };

  CommandMap["exit"] = [this](const std::vector<std::string> &args) {
    if (args.empty()) {
      exit(0);
    } else {
      exit(std::stoi(args[0]));
    }
    return "";
  };

  CommandMap["set"] = [this](const std::vector<std::string> &args) {
    if (args.size() < 2) {
      return "set: missing argument";
    }
    set(args[0], args[1]);
    return "";
  };
  CommandMap["unset"] = [this](const std::vector<std::string> &args) {
    if (args.empty()) {
      return "unset: missing argument";
    }
    unset(args[0]);
    return "";
  };

  CommandMap["history"] = [this](const std::vector<std::string> &args) {
    (void)args;
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

  tokenizer.setInput(Buffer);
  std::vector<Token> tokens = tokenizer.tokenize();

#ifdef _DEBUG
  for (const auto &t : tokens) {
    std::cout << "Token: { " << tokenTypeToString(t.type) << ", " << t.value
              << " }" << std::endl;
  }
#endif

  if (tokens.empty()) {
    return;
  }

  Token cmd = tokens[0];
  std::vector<std::string> args;
  for (size_t i = 1; i < tokens.size(); i++) {

    args.push_back(tokens[i].value);
  }

  addToCommandHistory(Buffer);
  Buffer.clear();

  auto it = CommandMap.find(cmd.value);
  if (it != CommandMap.end()) {
    std::string output = it->second(args);
    if (!output.empty()) {
      std::cout << output << std::endl;
    }
  } else {
    int status = executeCommand(cmd.value, args);
    if (status != 0) {
      std::cerr << "Command failed with status: " << status << std::endl;
    }
  }
}

int Shell::executeCommand(const std::string &command,
                          const std::vector<std::string> &args) {
  int pid = fork();
  if (pid == 0) {
    // Child process
    char **env = getEnviron();

    char **argv = new char *[args.size() + 2];
    argv[0] = new char[command.size() + 1];
    std::strcpy(argv[0], command.c_str());

    for (size_t i = 0; i < args.size(); i++) {
      argv[i + 1] = new char[args[i].size() + 1];
      std::strcpy(argv[i + 1], args[i].c_str());
    }
    argv[args.size() + 1] = nullptr;

// debug print
#ifdef _DEBUG
    std::cout << "Executing command: " << command << std::endl;
    for (size_t i = 0; argv[i] != nullptr; i++) {
      std::cout << "argv[" << i << "]: " << argv[i] << std::endl;
    }
#endif

    execvpe(command.c_str(), argv, env);

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

void Shell::exit(const int &code) {
  termiosExitRawMode();
  ::exit(code);
}

void Shell::set(const std::string &key, const std::string &value) {
  Environment[key] = value;
}

void Shell::unset(const std::string &key) { Environment.erase(key); }

void Shell::addToCommandHistory(const std::string &command) {
  CommandHistory.push_back(command);
  CommandHistoryIndex = CommandHistory.size();
}

void Shell::showCommandHistory() {
  for (size_t i = 0; i < CommandHistory.size(); i++) {
    std::cout << i << ": " << CommandHistory[i] << std::endl;
  }
}