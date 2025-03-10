#pragma once
#include <string>
#include <vector>

enum class TokenType {
  COMMAND,
  STRING,
  PIPE,
  REDIRECT,
  BACKGROUND,
  SEMICOLON,
  AND,
  OR,
  SUBSHELL,
  SUBSHELL_END,
  END_OF_FILE
};

class Token {
public:
  std::string value;
  TokenType type;

  Token(const std::string &val, TokenType t) : value(val), type(t) {}
};

class Tokenizer {
public:
  std::vector<Token> tokens;

  void addToken(const std::string &value, TokenType type);
  std::vector<Token> parseTokens(const std::string &input);
  void clearTokens();
};