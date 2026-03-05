#ifndef PROCESS_CONTEXT_H
#define PROCESS_CONTEXT_H

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <glm/glm.hpp>

namespace PCG {

struct ProcessContext {
  // Position being evaluated
  glm::vec2 position_;

  // Input/output buffers
  std::vector<float*> inputs_;
  std::vector<float*> outputs_;

  // Node parameters
  const std::vector<std::variant<float, int, bool, std::string>>* params_;

  // Resource cache (for noise instances, etc.)
  // ! MOVE TO GRAPH GENERATOR
  std::unordered_map<std::string, std::shared_ptr<void>>* resources_;

  // Accessors used by process_func
  glm::vec2 position() const { return position_; }
  float get_input(size_t i) const { return *inputs_[i]; }
  void set_output(size_t i, float v) { *outputs_[i] = v; }

  template <typename T>
  T get_param(size_t i) const {
    return std::get<T>((*params_)[i]);
  }

  template <typename T, typename... Args>
  T* get_or_create_resource(const std::string& key, Args&&... args) {
    auto it = resources_->find(key);
    if (it != resources_->end())
      return static_cast<T*>(it->second.get());

    auto ptr = std::make_shared<T>(std::forward<Args>(args)...);
    T* raw = ptr.get();
    (*resources_)[key] = ptr;
    return raw;
  }
};

}  // namespace PCG

#endif  // PROCESS_CONTEXT_H