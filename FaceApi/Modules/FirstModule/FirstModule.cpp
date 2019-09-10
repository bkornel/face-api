#include "Modules/FirstModule/FirstModule.h"
#include "Messages/CommandMessage.h"

#include "Framework/Functional.hpp"

namespace face
{
  FirstModule::FirstModule() :
    mExecutor(fw::getInlineExecutor())
  {
    Connect();
  }

  fw::ErrorCode FirstModule::Connect()
  {
    auto result = fw::connect(FW_BIND(&FirstModule::Main, this), mExecutor);
    mFunction = result.first;
    mOutputPort = result.second;
    return fw::ErrorCode::OK;
  }

  unsigned FirstModule::Main(bool)
  {
    return mTickCounter++;
  }

  void FirstModule::Tick()
  {
    mFunction();
  }

  void FirstModule::RunFaceDetector()
  {
    sCommand.Raise(std::make_shared<CommandMessage>(CommandMessage::Type::RunFaceDetection, mTickCounter, fw::get_current_time()));
  }
}
