#ifndef KEY_MAP_FTXUI_H
#define KEY_MAP_FTXUI_H
#include <ftxui/component/component.hpp>
#include "key.h"

// Not sure if this is the correct way
inline Key ftxui_key_to_key(const ftxui::Event& event) {
  const char c = event.character()[0];

  // Handle uppercase letters
  if (c >= 'A' && c <= 'Z')
    return static_cast<Key>(c);

  // Handle lowercase letters by converting to uppercase
  if (c >= 'a' && c <= 'z')
    return static_cast<Key>(c - ('a' - 'A'));

  // Handle digits
  if (c >= '0' && c <= '9')
    return static_cast<Key>(c);

  // Handle special characters
  switch (c) {
    case ' ':
      return Key::SPACE;
    case '!':
      return Key::EXCLAMATION;
    case '"':
      return Key::DOUBLE_QUOTE;
    case '#':
      return Key::HASH;
    case '$':
      return Key::DOLLAR;
    case '%':
      return Key::PERCENT;
    case '&':
      return Key::AMPERSAND;
    case '\'':
      return Key::SINGLE_QUOTE;
    case '(':
      return Key::LEFT_PARENTHESIS;
    case ')':
      return Key::RIGHT_PARENTHESIS;
    case '*':
      return Key::ASTERISK;
    case '+':
      return Key::PLUS;
    case ',':
      return Key::COMMA;
    case '-':
      return Key::MINUS;
    case '.':
      return Key::PERIOD;
    case '/':
      return Key::SLASH;
    case ':':
      return Key::COLON;
    case ';':
      return Key::SEMICOLON;
    case '<':
      return Key::LESS_THAN;
    case '=':
      return Key::EQUALS;
    case '>':
      return Key::GREATER_THAN;
    case '?':
      return Key::QUESTION;
    case '@':
      return Key::AT;
    case '[':
      return Key::LEFT_BRACKET;
    case '\\':
      return Key::BACKSLASH;
    case ']':
      return Key::RIGHT_BRACKET;
    case '^':
      return Key::CARET;
    case '_':
      return Key::UNDERSCORE;
    case '`':
      return Key::BACKTICK;
    case '{':
      return Key::LEFT_CURLY;
    case '|':
      return Key::PIPE;
    case '}':
      return Key::RIGHT_CURLY;
    case '~':
      return Key::TILDE;
    case 127:
      return Key::DELETE;  // ASCII delete
  }

  // Handle special keys (arrows, etc.)
  if (event == ftxui::Event::ArrowRight)
    return Key::RIGHT_ARROW;
  if (event == ftxui::Event::ArrowLeft)
    return Key::LEFT_ARROW;
  if (event == ftxui::Event::ArrowUp)
    return Key::UP_ARROW;
  if (event == ftxui::Event::ArrowDown)
    return Key::DOWN_ARROW;

  return Key::NONE;  // Default case
}

#endif  // KEY_MAP_H