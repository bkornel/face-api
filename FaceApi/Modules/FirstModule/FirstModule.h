#pragma once

#include "Framework/Module.h"
#include "Framework/Port.hpp"

namespace face
{
  /// @brief The source of the module graph: it has no predecessor, so every frame starts
  /// with a Tick() rather than with an incoming message.
  class FirstModule : public fw::Module,
                      public fw::Port<unsigned()>
  {
  public:
    FW_DEFINE_SMART_POINTERS(FirstModule);

    FirstModule() = default;

    ~FirstModule() override = default;

    unsigned Main() override;

    void Tick();

    void Clear() override
    {
      mTickCounter = 0U;
    }

  private:
    unsigned mTickCounter = 0U;
  };
}
