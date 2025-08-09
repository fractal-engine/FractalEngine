#ifndef COMMAND_QUEUE_H
#define COMMAND_QUEUE_H

#include <functional>
#include <mutex>
#include <vector>

// This namespace provides a clean, global interface for queueing commands
// that need to be run safely on the main thread.
namespace CommandQueue {

// A simple vector to hold our commands (functions).
inline std::vector<std::function<void()>> g_main_thread_commands;

// A mutex to make sure that adding to the vector is thread-safe.
inline std::mutex g_command_mutex;

// The "Producer" function. The UI will call this.
// It safely adds a new command to the back of the queue.
inline void Enqueue(std::function<void()> command) {
  std::lock_guard<std::mutex> lock(g_command_mutex);
  g_main_thread_commands.push_back(command);
}

}  // namespace CommandQueue

#endif  // COMMAND_QUEUE_H