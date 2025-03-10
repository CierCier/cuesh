#pragma once

enum class CommandType {
  CD,
  EXIT,
  CLEAR,
  EXTERNAL // For commands found in $PATH
};
