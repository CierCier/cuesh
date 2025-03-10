#include "cuesh.hpp"
#include <cstring>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

Shell::Shell() {
  // Initialize built-in commands
  commandMap["cd"] = [this](const std::vector<Token> &tokens) {
    return cd(tokens);
  };
  commandMap["exit"] = [this](const std::vector<Token> &tokens) {
    return quit(tokens);
  };
  commandMap["clear"] = [this](const std::vector<Token> &tokens) {
    return clear(tokens);
  };

  // Initialize working directory
  char *cwdBuffer = getcwd(nullptr, 0);
  if (cwdBuffer) {
    env["PWD"] = cwdBuffer;
    free(cwdBuffer);
  }

  // Enable raw mode for terminal
  enableRawMode();

  // initialize tokenizer
  tokenizer = Tokenizer();

  // update environ for execvpe
  cEnviron = nullptr;

  char *e = *environ;
  while (e) {
    std::string envVar(e);
    size_t pos = envVar.find("=");
    std::string key = envVar.substr(0, pos);
    std::string value = envVar.substr(pos + 1);
    setEnv(key, value);
    e = *(++environ);
  }

  // initialize some default environment variables
  char hostname[256];
  gethostname(hostname, 256);
  setEnv("hostname", hostname);
  setEnv("PS1", "$USER@$hostname:$PWD > ");
  // Run the shell
  init();
}

int Shell::runCommand(const std::vector<Token> &tokens) {
  if (tokens.empty()) {
    return 0;
  }

  std::string command = tokens[0].value;

  // Check for built-in command
  auto it = commandMap.find(command);
  if (it != commandMap.end()) {
    return it->second(tokens);
  }

  // Otherwise, execute as an external command
  return executeExternalCommand(tokens);
}

// Change directory (builtin)
int Shell::cd(const std::vector<Token> &tokens) {
  if (tokens.size() < 2) {
    std::cerr << "cd: missing argument\n";
    return 1;
  }

  if (chdir(tokens[1].value.c_str()) != 0) {
    perror("cd");
    return 1;
  }

  // Update PWD in the environment
  char *cwdBuffer = getcwd(nullptr, 0);
  if (cwdBuffer) {
    env["PWD"] = cwdBuffer;
    free(cwdBuffer);
  }

  return 0;
}

// Exit shell (builtin)
int Shell::quit(const std::vector<Token> &tokens) {
  (void)tokens;
  std::cout << "Exiting shell...\n";
  exit(0);
}

// Clear screen (builtin)
int Shell::clear(const std::vector<Token> &tokens) {
  (void)tokens;
  std::cout << "\033[2J\033[H"; // ANSI escape code to clear screen
  return 0;
}

// Execute external commands using execvpe
int Shell::executeExternalCommand(const std::vector<Token> &tokens) {
  std::vector<char *> args;
  for (const auto &token : tokens) {
    char *arg = const_cast<char *>(token.value.c_str());
    if (std::strlen(arg) > 0) {
      args.push_back(arg);
    }
  }
  args.push_back(nullptr); // execvp requires NULL-terminated argument list

  pid_t pid = fork();
  if (pid == 0) {
    // Child process
    int s = execvpe(args[0], args.data(), this->cEnviron);
    if (s == -1) {
      perror("execvp");
      return 1;
    }
    return 0;

  } else if (pid > 0) {
    // Parent process
    int status;
    waitpid(pid, &status, 0);
    return WEXITSTATUS(status);
  } else {
    std::cerr << "Error: Failed to fork process\n";
    return 1;
  }
}

// Helper to set an environment variable
void Shell::setEnv(const std::string &key, const std::string &value) {
  env[key] = value;

  // Update environ for execvpe
  auto it = env.find(key);
  if (it != env.end()) {
    char *envVar = new char[it->first.size() + it->second.size() + 2];
    std::strcpy(envVar, (it->first + "=" + it->second).c_str());
    cEnviron = const_cast<char **>(cEnviron);
    cEnviron = cEnviron ? cEnviron : new char *[2];
    cEnviron[0] = envVar;
    cEnviron[1] = nullptr;
  }

  if (key == "PS1") {
    prompt = replaceVariable(value);
  }
}

// Helper to get an environment variable
std::string Shell::getEnv(const std::string &key) {
  auto it = env.find(key);
  return (it != env.end()) ? it->second : "";
}

// Print shell prompt
void Shell::printPrompt() { std::cout << prompt << std::flush; }

// Enable raw mode for terminal
void Shell::enableRawMode() {
  struct termios raw;

  tcgetattr(STDIN_FILENO, &originalTermios);
  raw = originalTermios;

  // Disable canonical mode (line-by-line input) and echo
  raw.c_lflag &= ~(ECHO | ICANON);

  // Set minimum input bytes for read() to return
  raw.c_cc[VMIN] = 1;

  // Set timeout for read() to return
  raw.c_cc[VTIME] = 0;

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Disable raw mode for terminal
void Shell::disableRawMode() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &originalTermios);
}

// Handle signals for shell
void Shell::handleSignal(char c) {
  switch (c) {
  case 3: // Ctrl-C (SIGINT)
    inputBuffer.clear();
    std::cout << "\n^C\n";
    printPrompt(); // Show a new prompt after Ctrl-C
    break;

  case 4: // Ctrl-D (EOF)
    if (inputBuffer.empty()) {
      std::cout << "Exiting shell...\n";
      exit(0);
    }
    break;

  case 20: // Ctrl-T (Swap last two characters)
    if (inputBuffer.size() >= 2) {
      std::swap(inputBuffer[inputBuffer.size() - 1],
                inputBuffer[inputBuffer.size() - 2]);
      std::cout << "\b\b" << inputBuffer[inputBuffer.size() - 2]
                << inputBuffer[inputBuffer.size() - 1] << std::flush;
    }
    break;

  case 21: // Ctrl-U (Clear line)
    while (!inputBuffer.empty()) {
      std::cout << "\b \b"; // Move back, clear, move back
      inputBuffer.pop_back();
      std::cout << std::flush;
    }
    break;

  case 127: // Backspace
    if (!inputBuffer.empty()) {
      inputBuffer.pop_back();
      std::cout << "\b \b" << std::flush; // Move back, clear, move back
    }
    break;

  default:
    inputBuffer += c;
    std::cout << c << std::flush; // Print normally
    break;
  }
}

// Initialize shell and run main loop
int Shell::init() {
  std::cout << "Initializing shell...\n";

  while (true) {
    printPrompt();
    int status = run();
    setEnv("?", std::to_string(status));
  }
}

// Main shell loop
int Shell::run() {
  char c;
  while (read(STDIN_FILENO, &c, 1) == 1) {
    if (c == '\n') {
      std::cout << '\n';
      tokens = tokenizer.parseTokens(inputBuffer);
      replaceVariables(tokens);
      int status = runCommand(tokens);
      inputBuffer.clear();
      return status;
    } else {
      handleSignal(c);
    }
  }

  return 0;
}
std::string Shell::replaceVariable(const std::string &value) {
  std::string result;
  size_t pos = 0;
  while (pos < value.size()) {
    if (value[pos] == '$') {
      size_t end = pos + 1;
      while (end < value.size() && (isalnum(value[end]) || value[end] == '_')) {
        ++end;
      }
      std::string key = value.substr(pos + 1, end - pos - 1);
      if (key == "$") {
        result += std::to_string(getpid());
      } else {
        result += getEnv(key);
      }
      pos = end;
    } else {
      result += value[pos];
      ++pos;
    }
  }
  return result;
}

void Shell::replaceVariables(std::vector<Token> &tokens) {
  for (auto &token : tokens) {
    if (token.type == TokenType::STRING || token.type == TokenType::COMMAND) {
      token.value = replaceVariable(token.value);
    }
  }
}