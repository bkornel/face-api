#include "Modules/FirstModule/FirstModule.h"

namespace face
{
  uint32_t FirstModule::Main()
  {
    DrainCommands();

    return mTickCounter++;
  }

  void FirstModule::Tick()
  {
    Trigger();
  }
}
