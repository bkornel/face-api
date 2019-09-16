#include "Modules/ModuleConnector.h"

#include "Framework/UtilString.h"

#include "Modules/FaceDetection/FaceDetection.h"
#include "Modules/FirstModule/FirstModule.h"
#include "Modules/ImageQueue/ImageQueue.h"
#include "Modules/LastModule/LastModule.h"
#include "Modules/UserHistory/UserHistory.h"
#include "Modules/UserManager/UserManager.h"
#include "Modules/UserProcessor/UserProcessor.h"
#include "Modules/Visualizer/Visualizer.h"

#include <easyloggingpp/easyloggingpp.h>

namespace face
{
  namespace
  {
    template<typename T1, typename T2>
    bool set_input_port(T1 iModule, fw::Module::Shared iPredecessor, int iPortNo)
    {
      CV_DbgAssert(iModule && iPredecessor);

      std::shared_ptr<T2> derived = std::dynamic_pointer_cast<T2>(iPredecessor);
      if (!derived)
      {
        return false;
      }

      return iModule->SetInputPort(derived->GetOutputPort(), iPortNo - 1) == fw::ErrorCode::OK;
    }

    template<typename T>
    bool connect(fw::Module::Shared iModule, const ModuleConnector::PredecessorMap& iPredecessors)
    {
      CV_DbgAssert(iModule);

      using TShared = std::shared_ptr<T>;

      TShared derived = std::dynamic_pointer_cast<T>(iModule);
      if (!derived)
      {
        return false;
      }

      for (const auto& predecessor : iPredecessors)
      {
        if (!predecessor.second)
        {
          LOG(ERROR) << "Predecessor is not defined for module: " << iModule->GetName();
          return false;
        }

        if (set_input_port<TShared, FaceDetection>(derived, predecessor.second, predecessor.first)) continue;
        if (set_input_port<TShared, FirstModule>(derived, predecessor.second, predecessor.first)) continue;
        if (set_input_port<TShared, ImageQueue>(derived, predecessor.second, predecessor.first)) continue;
        if (set_input_port<TShared, UserHistory>(derived, predecessor.second, predecessor.first)) continue;
        if (set_input_port<TShared, UserManager>(derived, predecessor.second, predecessor.first)) continue;
        if (set_input_port<TShared, UserProcessor>(derived, predecessor.second, predecessor.first)) continue;
        if (set_input_port<TShared, Visualizer>(derived, predecessor.second, predecessor.first)) continue;
        // REMARK: Insert new modules here

        LOG(ERROR) << "Connection cannot be created: " << predecessor.second->GetName() << " -> " << iModule->GetName();
        return false;
      }

      derived->Connect();

      return true;
    }
  }

  fw::ErrorCode ModuleConnector::Connect(fw::Module::Shared iModule, const PredecessorMap& iPredecessors)
  {
    CV_DbgAssert(iModule);

    std::stringstream ss;
    ss << "Predecessors of [" << iModule->GetName() << "]:\t";

    if (connect<FaceDetection>(iModule, iPredecessors)) return fw::ErrorCode::OK;
    if (connect<ImageQueue>(iModule, iPredecessors)) return fw::ErrorCode::OK;
    if (connect<LastModule>(iModule, iPredecessors)) return fw::ErrorCode::OK;
    if (connect<UserHistory>(iModule, iPredecessors)) return fw::ErrorCode::OK;
    if (connect<UserManager>(iModule, iPredecessors)) return fw::ErrorCode::OK;
    if (connect<UserProcessor>(iModule, iPredecessors)) return fw::ErrorCode::OK;
    if (connect<Visualizer>(iModule, iPredecessors)) return fw::ErrorCode::OK;
    // REMARK: Insert new modules here

    LOG(INFO) << ss.str();

    return fw::ErrorCode::OK;
  }
}
