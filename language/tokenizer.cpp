#include "tokenizer.hpp"
#include <cctype>
#include <stdexcept>

void Tokenizer::skipWhitespace() {
  while (hasMoreTokens() && std::isspace(input[pos])) {
    if (input[pos] == '\n') {
      line++;
      column = 1;
    } else {
      column++;
    }
    pos++;
  }
}

void Tokenizer::skipComment() {
  if (input[pos] == '#') {
    while (hasMoreTokens() && input[pos] != '\n') {
      pos++;
      column++;
    }
  }
}

bool Tokenizer::hasMoreTokens() { return pos < input.length(); }

Token Tokenizer::nextToken() {
  skipWhitespace();
  skipComment();

  if (!hasMoreTokens()) {
    return Token(TokenType::TKN_EOF, "", line, column);
  }

  char c = input[pos];
  size_t startLine = line;
  size_t startColumn = column;

  // Handle string literals
  if (c == '"' || c == '\'') {
    char quote = c;
    pos++;
    column++;
    size_t start = pos;
    while (hasMoreTokens() && input[pos] != quote) {
      if (input[pos] == '\n') {
        throw std::runtime_error("Unclosed string literal");
      }
      pos++;
      column++;
    }
    if (!hasMoreTokens()) {
      throw std::runtime_error("Unclosed string literal");
    }
    std::string value = input.substr(start, pos - start);
    pos++;
    column++;
    return Token(TokenType::TKN_STRING_LITERAL, value, startLine, startColumn);
  }

  // Handle numbers
  if (std::isdigit(c)) {
    size_t start = pos;
    while (hasMoreTokens() && std::isdigit(input[pos])) {
      pos++;
      column++;
    }
    return Token(TokenType::TKN_NUMBER_LITERAL,
                 input.substr(start, pos - start), startLine, startColumn);
  }

  // Handle identifiers and keywords
  if (std::isalpha(c) || c == '_') {
    size_t start = pos;
    while (hasMoreTokens() && (std::isalnum(input[pos]) || input[pos] == '_')) {
      pos++;
      column++;
    }
    std::string value = input.substr(start, pos - start);
    if (value == "and") {
      return Token(TokenType::TKN_LOGICAL_AND, value, startLine, startColumn);
    } else if (value == "or") {
      return Token(TokenType::TKN_LOGICAL_OR, value, startLine, startColumn);
    } else if (value == "not") {
      return Token(TokenType::TKN_NOT, value, startLine, startColumn);
    }
    return Token(TokenType::TKN_IDENTIFIER, value, startLine, startColumn);
  }

  // Handle operators and special characters
  switch (c) {
  case '+':
  case '-':
  case '*':
  case '/':
  case '%':
    pos++;
    column++;
    return Token(TokenType::TKN_OPERATOR, std::string(1, c), startLine,
                 startColumn);
  case '=':
    if (input[pos + 1] == '=') {
      pos += 2;
      column += 2;
      return Token(TokenType::TKN_COMPARISON, "==", startLine, startColumn);
    }
    pos++;
    column++;
    return Token(TokenType::TKN_ASSIGNMENT, "=", startLine, startColumn);
  case '!':
    if (input[pos + 1] == '=') {
      pos += 2;
      column += 2;
      return Token(TokenType::TKN_COMPARISON, "!=", startLine, startColumn);
    }
    throw std::runtime_error("Unexpected character '!' without '='");
  case '<':
  case '>':
    if (input[pos + 1] == '=') {
      std::string op = std::string(1, c) + "=";
      pos += 2;
      column += 2;
      return Token(TokenType::TKN_COMPARISON, op, startLine, startColumn);
    }
    pos++;
    column++;
    return Token(TokenType::TKN_COMPARISON, std::string(1, c), startLine,
                 startColumn);
  case '(':
    pos++;
    column++;
    return Token(TokenType::TKN_LPAREN, "(", startLine, startColumn);
  case ')':
    pos++;
    column++;
    return Token(TokenType::TKN_RPAREN, ")", startLine, startColumn);
  case '{':
    pos++;
    column++;
    return Token(TokenType::TKN_LBRACE, "{", startLine, startColumn);
  case '}':
    pos++;
    column++;
    return Token(TokenType::TKN_RBRACE, "}", startLine, startColumn);
  case '[':
    pos++;
    column++;
    return Token(TokenType::TKN_LBRACKET, "[", startLine, startColumn);
  case ']':
    pos++;
    column++;
    return Token(TokenType::TKN_RBRACKET, "]", startLine, startColumn);
  case ':':
    pos++;
    column++;
    return Token(TokenType::TKN_COLON, ":", startLine, startColumn);
  case ',':
    pos++;
    column++;
    return Token(TokenType::TKN_COMMA, ",", startLine, startColumn);
  case '.':
    pos++;
    column++;
    return Token(TokenType::TKN_DOT, ".", startLine, startColumn);
  case '\n':
    pos++;
    line++;
    column = 1;
    return Token(TokenType::TKN_NEWLINE, "\n", startLine, startColumn);
  default:
    throw std::runtime_error("Unexpected character: " + std::string(1, c));
  }
}

std::vector<Token> Tokenizer::tokenize() {
  std::vector<Token> tokens;
  while (hasMoreTokens()) {
    tokens.push_back(nextToken());
  }
  tokens.push_back(Token(TokenType::TKN_EOF, "", line, column));
  return tokens;
}

const std::string tokenTypeToString(TokenType type) {
  switch (type) {
  case TokenType::TKN_IDENTIFIER:
    return "TKN_IDENTIFIER";
  case TokenType::TKN_NUMBER_LITERAL:
    return "TKN_NUMBER_LITERAL";
  case TokenType::TKN_STRING_LITERAL:
    return "TKN_STRING_LITERAL";
  case TokenType::TKN_OPERATOR:
    return "TKN_OPERATOR";
  case TokenType::TKN_COMPARISON:
    return "TKN_COMPARISON";
  case TokenType::TKN_ASSIGNMENT:
    return "TKN_ASSIGNMENT";
  case TokenType::TKN_LOGICAL_AND:
    return "TKN_LOGICAL_AND";
  case TokenType::TKN_LOGICAL_OR:
    return "TKN_LOGICAL_OR";
  case TokenType::TKN_NOT:
    return "TKN_NOT";
  case TokenType::TKN_LPAREN:
    return "TKN_LPAREN";
  case TokenType::TKN_RPAREN:
    return "TKN_RPAREN";
  case TokenType::TKN_LBRACE:
    return "TKN_LBRACE";
  case TokenType::TKN_RBRACE:
    return "TKN_RBRACE";
  case TokenType::TKN_LBRACKET:
    return "TKN_LBRACKET";
  case TokenType::TKN_RBRACKET:
    return "TKN_RBRACKET";
  case TokenType::TKN_COLON:
    return "TKN_COLON";
  case TokenType::TKN_COMMA:
    return "TKN_COMMA";
  case TokenType::TKN_DOT:
    return "TKN_DOT";
  case TokenType::TKN_NEWLINE:
    return "TKN_NEWLINE";
  case TokenType::TKN_EOF:
    return "TKN_EOF";
  case TokenType::TKN_COMMENT:
    return "TKN_COMMENT";
  default:
    return "TKN_NULL";
  }
}