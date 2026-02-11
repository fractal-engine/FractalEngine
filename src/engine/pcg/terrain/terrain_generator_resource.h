#ifndef TERRAIN_GENERATOR_RESOURCE_H
#define TERRAIN_GENERATOR_RESOURCE_H

#include "engine/memory/resource.h"
#include "engine/pcg/terrain/terrain_generator_base.h"

namespace PCG {

class GeneratorResource : public Resource {
public:
  GeneratorResource() = default;

  void SetGenerator(std::unique_ptr<GeneratorBase> generator) {
    generator_ = std::move(generator);
  }

  GeneratorBase* Get() { return generator_.get(); }
  const GeneratorBase* Get() const { return generator_.get(); }

  // Resource interface
  void Destroy() override { generator_.reset(); }
  std::string GetName() const {
    return generator_ ? generator_->GetDisplayName() : "Empty Generator";
  }

private:
  std::unique_ptr<GeneratorBase> generator_;
};

}  // namespace PCG

#endif  // TERRAIN_GENERATOR_RESOURCE_H