#include "Framework/ErrorCode.h"
#include "Modules/FacePipeline.h"

#include "Framework/Graph/IPortConnector.h"
#include "Framework/Profiler.h"
#include "Framework/Settings.h"
#include "Framework/Text.h"

#include "Modules/FaceDetection/FaceDetection.h"
#include "Modules/FaceTracker/FaceTracker.h"
#include "Modules/HeadPose/HeadPoseModule.h"
#include "Modules/ShapeModel/ShapeModelModule.h"
#include "Modules/ShapeNorm/ShapeNormModule.h"
#include "Modules/UserHistory/UserHistory.h"

#include "User/User.h"

#include <easyloggingpp/easyloggingpp.h>

namespace face
{
  // Upper bound for one frame, so a stalled graph cannot block the worker forever.
  const int64_t FacePipeline::sProcessTimeoutMs = 5000LL;

  FacePipeline::~FacePipeline()
  {
    DeInitialize();
  }

  fw::ErrorCode FacePipeline::Process()
  {
    if (!IsInitialized()) return fw::ErrorCode::BadState;

    FACE_PROFILER_FRAME_ID(GetLastFrameId());

    // One frame is one unit of failure: OpenCV reports what it dislikes by throwing, and the
    // worker has no handler above it, so an escaping exception would end the process.
    try
    {
      // Reading the generations before the tick is what makes this a per-frame barrier
      std::vector<uint64_t> generations;
      generations.reserve(mBarrier.size());

      for (const auto& sink : mBarrier)
        generations.emplace_back(sink->GetGeneration());

      mImageQueue->Trigger();

      // A frame is finished once every sink has published; failed nodes publish empty
      for (std::size_t i = 0U; i < mBarrier.size(); ++i)
      {
        if (!mBarrier[i]->WaitForNewValue(generations[i], fw::Milliseconds(sProcessTimeoutMs)))
        {
          LOG(WARNING) << "The module graph did not finish the frame within " << sProcessTimeoutMs << " ms.";
          return fw::ErrorCode::SystemFailure;
        }
      }

      CollectFrameOutput();
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

  void FacePipeline::Clear()
  {
    fw::ModuleGraph::Clear();

    std::lock_guard<std::mutex> lock(mLastMutex);
    mLastImage = nullptr;
    mLastUsers = nullptr;
  }

  void FacePipeline::CollectFrameOutput()
  {
    // The visualizer's frame carries the overlay; without one the queue's own frame is
    // what there is. Both are this frame's values: the graph is serialized per frame, so
    // the ports hold still until the next Trigger().
    std::shared_ptr<ImageMessage> image =
      mVisualizer ? mVisualizer->GetOutputPort()->Get() : mImageQueue->GetOutputPort()->Get();

    // Only a real frame is remembered: an empty round must not wipe what the host reads
    if (!image || image->IsEmpty()) return;

    std::shared_ptr<UserSnapshotMessage> users =
      mUserManager ? mUserManager->GetOutputPort()->Get() : nullptr;

    {
      std::lock_guard<std::mutex> lock(mLastMutex);
      mLastImage = image;
      mLastUsers = users;
    }

    mFrameProcessed.Raise(image);
  }

  fw::ErrorCode FacePipeline::InitializeInternal(const cv::FileNode& iModulesNode)
  {
    // The aliases are only ever assigned when empty, so drop the ones a previous
    // Initialize() left behind. Done here and not in DeInitialize(): they are read from
    // the app thread, and keeping every write inside Initialize() keeps those reads safe.
    mImageQueue = nullptr;
    mUserManager = nullptr;
    mVisualizer = nullptr;
    mBarrier.clear();

    {
      std::lock_guard<std::mutex> lock(mLastMutex);
      mLastImage = nullptr;
      mLastUsers = nullptr;
    }

    return fw::ModuleGraph::InitializeInternal(iModulesNode);
  }

  std::shared_ptr<fw::Module> FacePipeline::CreateModule(const cv::FileNode& iModuleNode)
  {
    const std::string& moduleName = fw::str::to_lower(iModuleNode.name());
    std::shared_ptr<fw::Module> newModule = nullptr;

    if (moduleName == "facedetection")
      newModule = std::make_shared<FaceDetection>();
    else if (moduleName == "facetracker")
      newModule = std::make_shared<FaceTracker>();
    else if (moduleName == "headpose")
      newModule = std::make_shared<HeadPoseModule>();
    else if (moduleName == "imagequeue")
      newModule = std::make_shared<ImageQueue>();
    else if (moduleName == "shapemodel")
      newModule = std::make_shared<ShapeModelModule>();
    else if (moduleName == "shapenorm")
      newModule = std::make_shared<ShapeNormModule>();
    else if (moduleName == "userhistory")
      newModule = std::make_shared<UserHistory>();
    else if (moduleName == "usermanager")
      newModule = std::make_shared<UserManager>();
    else if (moduleName == "visualizer")
      newModule = std::make_shared<Visualizer>();
    // REMARK: Insert new modules here

    // Check if the module is not set up in this file
    if (!newModule)
    {
      LOG(ERROR) << "Unknown module is referenced with name: " << iModuleNode.name();
      return nullptr;
    }

    // Model files in the settings are relative to the working directory
    newModule->SetWorkingDirectory(mWorkingDirectory);

    // Initialize the module (this will also load the module's settings)
    if (newModule->Initialize(iModuleNode) != fw::ErrorCode::OK)
    {
      return nullptr;
    }

    // The wiring. Handlers run on the raising thread and only post; the module applies the
    // work from Main(), where its state is owned. Every module reacts to the flag and to a
    // size change; what else a module listens to, or raises, is wired per type below.
    fw::Module* module = newModule.get();

    module->Listen(mEvents.verboseChanged, [module](bool iVerbose) {
      module->PostCommand([module, iVerbose] { module->SetVerboseMode(iVerbose); });
    });

    // A size change invalidates whatever a module cached about the frame
    module->Listen(mEvents.imageSizeChanged, [module](cv::Size /*iSize*/) {
      module->PostCommand([module] { module->Clear(); });
    });

    if (auto detector = std::dynamic_pointer_cast<FaceDetection>(newModule))
    {
      FaceDetection* raw = detector.get();

      raw->Listen(mEvents.runFaceDetection, [raw] {
        raw->PostCommand([raw] { raw->ForceDetection(); });
      });

      raw->Listen(mEvents.imageSizeChanged, [raw](cv::Size iSize) {
        raw->PostCommand([raw, iSize] { raw->OnImageSizeChanged(iSize); });
      });
    }

    if (auto tracker = std::dynamic_pointer_cast<FaceTracker>(newModule))
    {
      tracker->SetDetectionRequest([this] { mEvents.runFaceDetection.Raise(); });
    }

    if (auto imageQueue = std::dynamic_pointer_cast<ImageQueue>(newModule))
    {
      imageQueue->SetSizeChangedAnnouncer([this](cv::Size iSize) { mEvents.imageSizeChanged.Raise(iSize); });
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

  bool FacePipeline::IsObsoleteModule(const std::string& iModuleName) const
  {
    // The graph used to need a designated first and last module; the image queue is the
    // source now and the sinks are found from the wiring. Settings files still naming them
    // keep working - the modules are simply not created.
    return iModuleName == "firstmodule" || iModuleName == "lastmodule";
  }

  void FacePipeline::OnModuleCreated(const std::shared_ptr<fw::Module>& iModule)
  {
    // Create an alias for the modules whose output the host reads back
    // Duplications are already checked by the base class
    if (!mImageQueue) mImageQueue = std::dynamic_pointer_cast<ImageQueue>(iModule);

    if (!mUserManager) mUserManager = std::dynamic_pointer_cast<UserManager>(iModule);

    if (!mVisualizer) mVisualizer = std::dynamic_pointer_cast<Visualizer>(iModule);
  }

  fw::ErrorCode FacePipeline::ValidateModules()
  {
    if (!mImageQueue)
    {
      LOG(ERROR) << "Image queue module is not defined.";
      return fw::ErrorCode::BadData;
    }

    return fw::ErrorCode::OK;
  }

  void FacePipeline::OnGraphConnected()
  {
    mBarrier.clear();

    for (const auto& sink : GetSinkModules())
    {
      if (auto ports = std::dynamic_pointer_cast<fw::IPortConnector>(sink))
      {
        if (std::shared_ptr<fw::IFuture> output = ports->GetOutput())
        {
          LOG(INFO) << "Frame barrier waits on [" << sink->GetName() << "]";
          mBarrier.emplace_back(std::move(output));
        }
      }
    }
  }

  fw::ErrorCode FacePipeline::GetLastResults(FaceResults& oResults) const
  {
    oResults.clear();

    std::shared_ptr<UserSnapshotMessage> users;
    {
      std::lock_guard<std::mutex> lock(mLastMutex);
      users = mLastUsers;
    }

    // A graph without a user manager simply has no per-user results to report
    if (!users || users->IsEmpty()) return fw::ErrorCode::NotFound;

    const auto& activeUsers = users->GetUsers();
    oResults.reserve(activeUsers.size());

    for (const auto& user : activeUsers)
    {
      if (!user) continue;

      FaceResult result;
      result.userId = user->GetUserId();
      result.frameId = users->GetFrameId();
      result.timestamp = fw::to_epoch_ms(users->GetTimestamp());

      result.status = user->GetStatus();

      // Against the frame's own timestamp rather than the wall clock, so a result that is
      // read late does not report an age the face never had
      result.ageSeconds = fw::elapsed(user->GetCreationTs(), users->GetTimestamp()).count() / 1000.0;

      result.faceRect = user->GetFaceRect();
      result.shape2D = user->GetShape2D();
      result.shape3D = user->GetShape3D();
      result.normShape2D = user->GetNormShape2D();
      result.normShape3D = user->GetNormShape3D();
      result.faceBox = user->GetFaceBox();
      result.rpy = user->GetRPY();
      result.position3D = user->GetPosition3D();
      result.expression = user->GetExpression();
      result.cameraMatrix = user->GetCameraMatrix();
      result.rvec = user->GetRvec();
      result.tvec = user->GetTvec();

      oResults.emplace_back(std::move(result));
    }

    return oResults.empty() ? fw::ErrorCode::NotFound : fw::ErrorCode::OK;
  }

  uint32_t FacePipeline::GetLastFrameId() const
  {
    std::lock_guard<std::mutex> lock(mLastMutex);
    return mLastImage ? mLastImage->GetFrameId() : 0U;
  }

  int64_t FacePipeline::GetLastTimestamp() const
  {
    std::lock_guard<std::mutex> lock(mLastMutex);
    return mLastImage ? fw::to_epoch_ms(mLastImage->GetTimestamp()) : 0LL;
  }
}
