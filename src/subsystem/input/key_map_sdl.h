#ifndef KEY_MAP_SDL_H
#define KEY_MAP_SDL_H

#include <SDL.h>

#ifdef DELETE
#undef DELETE
#endif

#ifdef ERROR
#undef ERROR
#endif

#ifdef DEBUG
#undef DEBUG
#endif

#include "key.h"

inline Key sdl_key_to_key(const SDL_Event& event) {
  // Only handle KEYDOWN or KEYUP. If it's something else, return NONE.
  if (event.type != SDL_KEYDOWN && event.type != SDL_KEYUP) {
    return Key::NONE;
  }

  // scancode is hardware-level code
  SDL_Scancode sc = event.key.keysym.scancode;
  switch (sc) {
    // --- WASD movement keys ---
    case SDL_SCANCODE_W:
      return Key::W;
    case SDL_SCANCODE_A:
      return Key::A;
    case SDL_SCANCODE_S:
      return Key::S;
    case SDL_SCANCODE_D:
      return Key::D;

    // --- Arrow keys ---
    case SDL_SCANCODE_UP:
      return Key::UP_ARROW;
    case SDL_SCANCODE_DOWN:
      return Key::DOWN_ARROW;
    case SDL_SCANCODE_LEFT:
      return Key::LEFT_ARROW;
    case SDL_SCANCODE_RIGHT:
      return Key::RIGHT_ARROW;

      // Add more scancodes here if needed

    default:

      break;
  }

  // If the above didn’t match, we fallback to checking keysym.sym
  SDL_Keycode keycode = event.key.keysym.sym;
  switch (keycode) {
    // --- Digits '0'..'9' ---
    case SDLK_0:
      return Key::DIGIT_0;
    case SDLK_1:
      return Key::DIGIT_1;
    case SDLK_2:
      return Key::DIGIT_2;
    case SDLK_3:
      return Key::DIGIT_3;
    case SDLK_4:
      return Key::DIGIT_4;
    case SDLK_5:
      return Key::DIGIT_5;
    case SDLK_6:
      return Key::DIGIT_6;
    case SDLK_7:
      return Key::DIGIT_7;
    case SDLK_8:
      return Key::DIGIT_8;
    case SDLK_9:
      return Key::DIGIT_9;

    // --- Basic punctuation & space ---
    case SDLK_SPACE:
      return Key::SPACE;
    case SDLK_EXCLAIM:
      return Key::EXCLAMATION;
    case SDLK_QUOTE:
      return Key::SINGLE_QUOTE;
    case SDLK_QUOTEDBL:
      return Key::DOUBLE_QUOTE;
    case SDLK_HASH:
      return Key::HASH;
    case SDLK_DOLLAR:
      return Key::DOLLAR;
    case SDLK_PERCENT:
      return Key::PERCENT;
    case SDLK_AMPERSAND:
      return Key::AMPERSAND;
    case SDLK_LEFTPAREN:
      return Key::LEFT_PARENTHESIS;
    case SDLK_RIGHTPAREN:
      return Key::RIGHT_PARENTHESIS;
    case SDLK_ASTERISK:
      return Key::ASTERISK;
    case SDLK_PLUS:
      return Key::PLUS;
    case SDLK_COMMA:
      return Key::COMMA;
    case SDLK_MINUS:
      return Key::MINUS;
    case SDLK_PERIOD:
      return Key::PERIOD;
    case SDLK_SLASH:
      return Key::SLASH;
    case SDLK_COLON:
      return Key::COLON;
    case SDLK_SEMICOLON:
      return Key::SEMICOLON;
    case SDLK_LESS:
      return Key::LESS_THAN;
    case SDLK_EQUALS:
      return Key::EQUALS;
    case SDLK_GREATER:
      return Key::GREATER_THAN;
    case SDLK_QUESTION:
      return Key::QUESTION;
    case SDLK_AT:
      return Key::AT;
    case SDLK_LEFTBRACKET:
      return Key::LEFT_BRACKET;
    case SDLK_BACKSLASH:
      return Key::BACKSLASH;
    case SDLK_RIGHTBRACKET:
      return Key::RIGHT_BRACKET;
    case SDLK_CARET:
      return Key::CARET;
    case SDLK_UNDERSCORE:
      return Key::UNDERSCORE;
    case SDLK_BACKQUOTE:
      return Key::BACKTICK;
    case '{':
      return Key::LEFT_CURLY;
    case '|':
      return Key::PIPE;
    case '}':
      return Key::RIGHT_CURLY;
    case '~':
      return Key::TILDE;
    case SDLK_DELETE:
      return Key::DELETE;

    default:
      break;
  }

  // If no match, return NONE.
  return Key::NONE;
}

#endif  // KEY_MAP_SDL_H
