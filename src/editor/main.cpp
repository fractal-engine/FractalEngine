#include "editor/runtime/runtime.h"

int main() {
  Logger::getInstance().Log(LogLevel::Info, "Starting Fractal Engine");

  Runtime::START_LOOP();

  return 0;
}