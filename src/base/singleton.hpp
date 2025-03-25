#ifndef SINGLETON_HPP
#define SINGLETON_HPP
#include <iostream>

// This class is used to generate singleton class. A singleton class can be
// easily created by inheriting this template class.
// Example:
//     If you want to create a singleton class `Logger`
// ```cpp
// class Logger: public Singleton<Logger> {
//     // Do your codes here...
// };
// ```
// If you want to use `Logger`:
// ```cpp
//   Logger::getInstance();
// ```
// To get the logger instance.
// Please do not inherit any singleton class. Do not override
// `getInstance()` function.
template <typename Derived>
class Singleton {

public:
  static Derived& getInstance() {
    static Derived instance_;
    return instance_;
  }

protected:
  Singleton() = default;
  Singleton(const Singleton&) = delete;
  Singleton(const Singleton&&) = delete;
  Singleton& operator=(const Singleton&) = delete;
  Singleton& operator=(const Singleton&&) = delete;
  ~Singleton() = default;

  // NOTE - No sure if this is needed.
  // private:
  //   void preventFromConstruction() = 0;
};
#endif  // SINGLETON_HPP