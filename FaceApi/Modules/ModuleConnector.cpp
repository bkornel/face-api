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
    bool set_input_port(T1 iModule, fw::Module::Shared iPredecessor)
    {
      T2::Shared derived = std::dynamic_pointer_cast<T2>(iPredecessor);
      if (!derived)
      {
        return false;
      }
        
      // TODO(kbertok): change concept
      //iModule->SetInputPort<0>(derived->GetOutputPort());

      return true;
    }

    template<typename T>
    bool connect(fw::Module::Shared iModule, const ModuleConnector::PredecessorMap& iPredecessors)
    {
      T::Shared derived = std::dynamic_pointer_cast<T>(iModule);
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

        if (set_input_port<T::Shared, FaceDetection>(derived, predecessor.second)) continue;
        if (set_input_port<T::Shared, FirstModule>(derived, predecessor.second)) continue;
        if (set_input_port<T::Shared, ImageQueue>(derived, predecessor.second)) continue;
        if (set_input_port<T::Shared, LastModule>(derived, predecessor.second)) continue;
        if (set_input_port<T::Shared, UserHistory>(derived, predecessor.second)) continue;
        if (set_input_port<T::Shared, UserManager>(derived, predecessor.second)) continue;
        if (set_input_port<T::Shared, UserProcessor>(derived, predecessor.second)) continue;
        if (set_input_port<T::Shared, Visualizer>(derived, predecessor.second)) continue;

        LOG(ERROR) << "Connection cannot be created: " << predecessor.second->GetName() << " -> " << iModule->GetName();
        return false;
      }

      // TODO(kbertok): change concept
      //derived->Connect();

      return true;
    }
  }

  fw::ErrorCode ModuleConnector::Connect(fw::Module::Shared iModule, const PredecessorMap& iPredecessors)
  {
    CV_Assert(iModule);

    std::stringstream ss;
    ss << "Predecessors of [" << iModule->GetName() << "]:\t";

    if (connect<FaceDetection>(iModule, iPredecessors)) return fw::ErrorCode::OK;
    if (connect<ImageQueue>(iModule, iPredecessors)) return fw::ErrorCode::OK;
    if (connect<LastModule>(iModule, iPredecessors)) return fw::ErrorCode::OK;
    if (connect<UserHistory>(iModule, iPredecessors)) return fw::ErrorCode::OK;
    if (connect<UserManager>(iModule, iPredecessors)) return fw::ErrorCode::OK;
    if (connect<UserProcessor>(iModule, iPredecessors)) return fw::ErrorCode::OK;
    if (connect<Visualizer>(iModule, iPredecessors)) return fw::ErrorCode::OK;

    LOG(INFO) << ss.str();

    //mFaceDetection->SetInputPort<0>(mImageQueue->GetOutputPort());
    //mFaceDetection->Connect();
    
    return fw::ErrorCode::OK;
  }
}
