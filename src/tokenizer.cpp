#include "tokenizer.hpp"
#include <sstream>

std::vector<Token> Tokenizer::parseTokens(const std::string &input) {
  std::vector<Token> tokens;
  std::istringstream stream(input);
  std::string word;

  while (stream >> word) {
    TokenType type = TokenType::COMMAND;

    // Handle special tokens
    if (word == "|")
      type = TokenType::PIPE;
    else if (word == ">")
      type = TokenType::REDIRECT;
    else if (word == "&")
      type = TokenType::BACKGROUND;
    else if (word == ";")
      type = TokenType::SEMICOLON;
    else if (word == "&&")
      type = TokenType::AND;
    else if (word == "||")
      type = TokenType::OR;
    else if (word == "(")
      type = TokenType::SUBSHELL;
    else if (word == ")")
      type = TokenType::SUBSHELL_END;

    tokens.emplace_back(word, type);
  }

  tokens.emplace_back("", TokenType::END_OF_FILE); // EOF marker
  return tokens;
}
