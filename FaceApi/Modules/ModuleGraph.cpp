#include "Framework/ErrorCode.h"
#include "Modules/ModuleGraph.h"

#include "Framework/Profiler.h"
#include "Modules/ModuleFactory.h"

#include <easyloggingpp/easyloggingpp.h>

namespace face
{
  // Upper bound for one frame, so a stalled graph cannot block the worker forever.
  const int64_t ModuleGraph::sProcessTimeoutMs = 5000LL;

  ModuleGraph::~ModuleGraph()
  {
    DeInitialize();
  }

  fw::ErrorCode ModuleGraph::Process()
  {
    if (!IsInitialized()) return fw::ErrorCode::BadState;

    DrainCommands();

    FACE_PROFILER_FRAME_ID(GetLastFrameId());

    // One frame is one unit of failure: OpenCV reports what it dislikes by throwing, and the
    // worker has no handler above it, so an escaping exception would end the process.
    try
    {
      // Reading the generation before the tick is what makes this a per-frame barrier.
      const uint64_t generation = mLastModule->GetGeneration();

      mFirstModule->Tick();

      if (!mLastModule->WaitForNewOutput(generation, sProcessTimeoutMs))
      {
        LOG(WARNING) << "The module graph did not finish the frame within " << sProcessTimeoutMs << " ms.";
        return fw::ErrorCode::SystemFailure;
      }

      // Pushing a debug frame if we have it
      if (mLastModule->HasOutput())
      {
        mFrameProcessed.Raise(mLastModule->GetLastImage());
      }
    }
    catch (const cv::Exception& iException)
    {
      LOG(ERROR) << "Dropping the frame, OpenCV failed inside the module graph: " << iException.what();
      return fw::ErrorCode::SystemFailure;
    }
    catch (const std::exception& iException)
    {
      LOG(ERROR) << "Dropping the frame, a module failed: " << iException.what();
      return fw::ErrorCode::SystemFailure;
    }

    return fw::ErrorCode::OK;
  }

  fw::ErrorCode ModuleGraph::InitializeInternal(const cv::FileNode& iModulesNode)
  {
    // The aliases are only ever assigned when empty, so drop the ones a previous
    // Initialize() left behind. Done here and not in DeInitialize(): they are read from
    // the app thread, and keeping every write inside Initialize() keeps those reads safe.
    mFirstModule = nullptr;
    mLastModule = nullptr;
    mImageQueue = nullptr;

    return fw::ModuleGraph::InitializeInternal(iModulesNode);
  }

  std::shared_ptr<fw::Module> ModuleGraph::CreateModule(const cv::FileNode& iModuleNode)
  {
    // Extend the factory if you add a new module
    return mBus ? ModuleFactory::Create(iModuleNode, *mBus) : nullptr;
  }

  void ModuleGraph::OnModuleCreated(const std::shared_ptr<fw::Module>& iModule)
  {
    // Create an alias for the modules with a role in Process()
    // Duplications are already checked by the base class
    if (!mFirstModule) mFirstModule = std::dynamic_pointer_cast<FirstModule>(iModule);

    if (!mLastModule) mLastModule = std::dynamic_pointer_cast<LastModule>(iModule);

    if (!mImageQueue) mImageQueue = std::dynamic_pointer_cast<ImageQueue>(iModule);
  }

  fw::ErrorCode ModuleGraph::ValidateModules()
  {
    // First-, and last modules are mandatory
    if (!mFirstModule)
    {
      LOG(ERROR) << "First module is not defined.";
      return fw::ErrorCode::BadData;
    }

    if (!mLastModule)
    {
      LOG(ERROR) << "Last module is not defined.";
      return fw::ErrorCode::BadData;
    }

    if (!mImageQueue)
    {
      LOG(ERROR) << "Image queue module is not defined.";
      return fw::ErrorCode::BadData;
    }

    return fw::ErrorCode::OK;
  }
}
