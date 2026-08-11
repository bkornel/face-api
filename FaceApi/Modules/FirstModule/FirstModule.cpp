#include "Modules/FirstModule/FirstModule.h"

namespace face
{
  unsigned FirstModule::Main()
  {
    DrainCommands();

    return mTickCounter++;
  }

  void FirstModule::Tick()
  {
    Trigger();
  }
}
