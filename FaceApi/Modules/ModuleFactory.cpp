#include "Framework/ErrorCode.h"
#include "Modules/ModuleFactory.h"

#include "Framework/Graph/IPortConnector.h"
#include "Framework/Settings.h"
#include "Framework/Text.h"

#include "Modules/FaceDetection/FaceDetection.h"
#include "Modules/FirstModule/FirstModule.h"
#include "Modules/HeadPose/HeadPoseModule.h"
#include "Modules/ImageQueue/ImageQueue.h"
#include "Modules/LastModule/LastModule.h"
#include "Modules/ShapeModel/ShapeModelModule.h"
#include "Modules/ShapeNorm/ShapeNormModule.h"
#include "Modules/UserHistory/UserHistory.h"
#include "Modules/UserManager/UserManager.h"
#include "Modules/UserSnapshot/UserSnapshot.h"
#include "Modules/Visualizer/Visualizer.h"

#include <easyloggingpp/easyloggingpp.h>

namespace face
{
  std::shared_ptr<fw::Module> ModuleFactory::Create(const cv::FileNode& iModuleNode, fw::MessageBus& ioBus)
  {
    const std::string& moduleName = fw::str::to_lower(iModuleNode.name());
    std::shared_ptr<fw::Module> newModule = nullptr;

    if (moduleName == "facedetection")
      newModule = std::make_shared<FaceDetection>();
    else if (moduleName == "firstmodule")
      newModule = std::make_shared<FirstModule>();
    else if (moduleName == "headpose")
      newModule = std::make_shared<HeadPoseModule>();
    else if (moduleName == "imagequeue")
      newModule = std::make_shared<ImageQueue>();
    else if (moduleName == "lastmodule")
      newModule = std::make_shared<LastModule>();
    else if (moduleName == "shapemodel")
      newModule = std::make_shared<ShapeModelModule>();
    else if (moduleName == "shapenorm")
      newModule = std::make_shared<ShapeNormModule>();
    else if (moduleName == "userhistory")
      newModule = std::make_shared<UserHistory>();
    else if (moduleName == "usermanager")
      newModule = std::make_shared<UserManager>();
    else if (moduleName == "usersnapshot")
      newModule = std::make_shared<UserSnapshot>();
    else if (moduleName == "visualizer")
      newModule = std::make_shared<Visualizer>();
    // REMARK: Insert new modules here

    // Check if the module is not set up in this file
    if (!newModule)
    {
      LOG(ERROR) << "Unknown module is referenced with name: " << iModuleNode.name();
      return nullptr;
    }

    // Attach before Initialize(): that is where the module subscribes. newModule doubles
    // as the lifetime token, so its subscriptions cannot outlive it.
    newModule->Attach(ioBus, newModule);

    // Initialize the module (this will also load the module's settings)
    if (newModule->Initialize(iModuleNode) != fw::ErrorCode::OK)
    {
      return nullptr;
    }

    // Still before Connect(), which the graph does once every module exists
    if (auto ports = std::dynamic_pointer_cast<fw::IPortConnector>(newModule))
    {
      std::string executor;
      if (fw::get_value(iModuleNode, "executor", executor))
      {
        ports->SetExecutor(fw::get_executor_by_name(executor));
        LOG(INFO) << "Module [" << newModule->GetName() << "] runs on the " << executor << " executor";
      }
    }

    return newModule;
  }
}
