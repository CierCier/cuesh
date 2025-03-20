#pragma once

#include <stddef.h>
#include <string>
#include <vector>

enum class TokenType {
  TKN_NULL,
  TKN_IDENTIFIER,     // variable/function/class names
  TKN_KEYWORD,        // if, else, def, class, etc.
  TKN_STRING_LITERAL, // "string" or 'string'
  TKN_NUMBER_LITERAL, // 123, 3.14, etc.
  TKN_OPERATOR,       // +, -, *, /, %, etc.
  TKN_ASSIGNMENT,     // =
  TKN_COMPARISON,     // ==, !=, <, >, <=, >=
  TKN_LOGICAL_AND,    // and
  TKN_LOGICAL_OR,     // or
  TKN_NOT,            // not
  TKN_LPAREN,         // (
  TKN_RPAREN,         // )
  TKN_LBRACE,         // {
  TKN_RBRACE,         // }
  TKN_LBRACKET,       // [
  TKN_RBRACKET,       // ]
  TKN_COLON,          // :
  TKN_COMMA,          // ,
  TKN_DOT,            // .
  TKN_NEWLINE,        // \n
  TKN_EOF,            // end of file
  TKN_COMMENT         // # comment
};

enum class TokenError {
  NONE,            // No error
  UNEXPECTED_CHAR, // Unexpected character encountered
  UNCLOSED_STRING, // String literal not closed
  INVALID_NUMBER,  // Invalid number format
  UNKNOWN_ERROR    // Catch-all for unknown errors
};

class Token {
public:
  TokenType type;
  std::string value; // the actual value of the token (e.g., "if", "123", "+")
  size_t line;       // line number in the source code
  size_t column;     // column number in the source code

  Token(TokenType type, std::string value, size_t line, size_t column)
      : type(type), value(value), line(line), column(column) {}

  ~Token() {}
};

class Tokenizer {
public:
  Tokenizer() {}

  ~Tokenizer() {}

  // sets the input for the tokenizer and starts tokenizing
  void setInput(const std::string &input) {
    this->input = input;
    this->pos = 0;
    this->line = 1;
    this->column = 1;
  }

  // returns the next token in the input
  // for buffer-like tokenizing
  Token nextToken();

  bool hasMoreTokens();

  // if you want to tokenize the input all at once
  std::vector<Token> tokenize();

private:
  std::string input;
  size_t pos;
  size_t line;
  size_t column;
  Token currentToken = Token(TokenType::TKN_NULL, "", 0, 0);

  void skipWhitespace();

  void skipComment();
};
