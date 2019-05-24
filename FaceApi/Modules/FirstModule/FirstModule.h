#pragma once

#include "Framework/FlowGraph.hpp"
#include "Framework/Module.h"
#include "Framework/Port.hpp"

#include <functional>

namespace face
{
  class FirstModule :
    public fw::Module,
    public fw::Port<unsigned(bool)>
  {
  public:
    FW_DEFINE_SMART_POINTERS(FirstModule);

    FirstModule();

    virtual ~FirstModule() = default;

    fw::ErrorCode Connect() override;

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
