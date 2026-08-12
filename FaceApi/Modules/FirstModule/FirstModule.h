#pragma once

#include "Framework/Graph/Module.h"
#include "Framework/Graph/Port.hpp"

#include <cstdint>

namespace face
{
  /// @brief The source of the module graph: it has no predecessor, so every frame starts
  /// with a Tick() rather than with an incoming message.
  class FirstModule : public fw::Module,
                      public fw::Port<uint32_t()>
  {
  public:

    FirstModule() = default;

    ~FirstModule() override = default;

    uint32_t Main() override;

    void Tick();

    void Clear() override
    {
      mTickCounter = 0U;
    }

  private:
    uint32_t mTickCounter = 0U;
  };
}
