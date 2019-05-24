#include "Modules/ModuleFactory.h"

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
  fw::Module::Shared ModuleFactory::Create(const cv::FileNode& iModuleNode)
  {
    const std::string& moduleName = fw::str::to_lower(iModuleNode.name());
    fw::Module::Shared newModule = nullptr;

    if (moduleName == "facedetection")	    newModule = std::make_shared<FaceDetection>();
    else if (moduleName == "firstmodule")		newModule = std::make_shared<FirstModule>();
    else if (moduleName == "imagequeue")	  newModule = std::make_shared<ImageQueue>();
    else if (moduleName == "lastmodule")		newModule = std::make_shared<LastModule>();
    else if (moduleName == "userhistory")		newModule = std::make_shared<UserHistory>();
    else if (moduleName == "usermanager")		newModule = std::make_shared<UserManager>();
    else if (moduleName == "userprocessor")	newModule = std::make_shared<UserProcessor>();
    else if (moduleName == "visualizer")		newModule = std::make_shared<Visualizer>();

    // Check if the module is not set up in this file
    if (!newModule)
    {
      LOG(ERROR) << "Unknown module is referenced with name: " << iModuleNode.name();
      return nullptr;
    }

    // Initialize the module (this will also load the module's settings)
    if (newModule->Initialize(iModuleNode) != fw::ErrorCode::OK)
    {
      return nullptr;
    }

    return newModule;
  }
}
