#pragma once

#include "Modules/General/ModuleWithPort.hpp"
#include "Framework/FlowGraph.hpp"

#include <functional>

namespace face
{
  class FirstModule :
    public ModuleWithPort<unsigned(bool)>
  {
  public:
    FirstModule();

    virtual ~FirstModule() = default;

    void Connect() override;

    unsigned Main(bool);

    void Tick();

    void RunFaceDetector();

    void Clear() override
    {
      mTickCounter = 0U;
    }

  private:
    unsigned mTickCounter = 0U;
    std::function<void()> mFunction;
    fw::Executor::Shared mExecutor = nullptr;
  };
}
