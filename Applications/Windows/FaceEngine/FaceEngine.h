/// @file FaceEngine.h
/// @brief The flat C interface FaceStudio talks to.
///
/// Everything expensive lives behind this header: capture, the FaceApi pipeline and both
/// Direct3D renderers. The managed side never sees a pixel - it binds the swap chains this
/// engine hands out to its SwapChainPanels and polls one small struct per UI tick for the
/// numbers its panels display. That is what keeps the interop cost independent of the frame
/// size and of the frame rate.
///
/// Every struct here is blittable and laid out so that the C# declarations in
/// FaceStudio/Interop can mirror it field by field with the default sequential layout.

#pragma once

#include <stdint.h>

#ifdef FACEENGINE_EXPORTS
  #define FE_API __declspec(dllexport)
#else
  #define FE_API __declspec(dllimport)
#endif

#define FE_CALL __stdcall

#ifdef __cplusplus
extern "C" {
#endif

  enum
  {
    /// @brief Faces reported per snapshot. The pipeline's own maxTracks is normally lower.
    FE_MAX_FACES = 8,

    /// @brief Pipeline stages reported by the profiler
    FE_MAX_STAGES = 16
  };

  typedef enum FeStatus
  {
    FeStatus_Ok = 0,
    FeStatus_Failed = 1,
    FeStatus_InvalidArgument = 2,
    FeStatus_NotInitialized = 3,
    FeStatus_SourceFailed = 4,
    FeStatus_NoData = 5,
    FeStatus_Unsupported = 6
  } FeStatus;

  typedef enum FeSourceKind
  {
    FeSourceKind_None = 0,
    FeSourceKind_Camera = 1,
    FeSourceKind_File = 2
  } FeSourceKind;

  /// @brief Mirrors face::TrackStatus value for value, so that the two never have to be
  /// translated - the pipeline is what knows this, and it is passed through unchanged.
  typedef enum FeTrackState
  {
    /// @brief Found by the detector on this frame
    FeTrackState_Detected = 0,

    /// @brief Carried by the tracker from an earlier detection
    FeTrackState_Tracked = 1,

    /// @brief Not seen any more
    FeTrackState_Inactive = 2
  } FeTrackState;

  /// @brief One face of the last completed frame, reduced to what a panel can display.
  ///
  /// The landmarks themselves are deliberately not here: they are consumed by the renderers
  /// on this side of the boundary, and marshalling 68 points per face per frame would buy
  /// the UI nothing it can show.
  typedef struct FeFace
  {
    /// @brief Seconds since this identity was first reported
    double ageSeconds;

    /// @brief Head orientation in degrees, as a reader expects to see it
    double yawDeg;
    double pitchDeg;
    double rollDeg;

    /// @brief Head position in the camera coordinate system, in the model's units
    double posX;
    double posY;
    double posZ;

    /// @brief Face rectangle in the pixel coordinates of the source frame
    double rectX;
    double rectY;
    double rectWidth;
    double rectHeight;

    /// @brief Expression measures derived from the landmarks, each normalised to [0, 1]
    double openMouth;
    double openEyeLeft;
    double openEyeRight;
    double browRaise;
    double smile;

    int32_t userId;

    /// @brief One of FeTrackState
    int32_t state;

    /// @brief Zero when the pose could not be estimated, in which case the angles are stale
    int32_t hasPose;

    int32_t reserved;
  } FeFace;

  /// @brief Everything the UI polls, in one copy.
  typedef struct FeSnapshot
  {
    /// @brief Capture timestamp of the last completed frame, epoch milliseconds
    int64_t timestampMs;

    uint64_t framesCaptured;
    uint64_t framesProcessed;

    /// @brief Frames pushed that the pipeline neither returned nor could still be holding.
    /// It drops them silently when its queue is full and does not report it, so this is
    /// what went in, has not come out, and no longer fits inside.
    uint64_t framesDropped;

    uint64_t framesRendered;

    /// @brief Frames per second pushed into the pipeline, measured over a sliding window
    double captureFps;

    /// @brief Frames per second coming out of the pipeline
    double pipelineFps;

    /// @brief Presents per second of the video view
    double renderFps;

    /// @brief Age of the last completed frame, from capture to completion
    double latencyMs;
    double latencyMinMs;
    double latencyMaxMs;
    double latencyAvgMs;
    double latencyP95Ms;

    /// @brief Milliseconds of each profiler stage of the last frame, names via FeEngine_GetStageName
    double stageMs[FE_MAX_STAGES];

    int32_t frameId;
    int32_t sourceWidth;
    int32_t sourceHeight;
    int32_t faceCount;

    /// @brief Frames pushed that have not come back out: what is inside the pipeline
    int32_t queueDepth;

    int32_t stageCount;

    int32_t isRunning;
    int32_t isPaused;

    /// @brief One of FeSourceKind
    int32_t sourceKind;

    /// @brief Non-zero while the pipeline's own Visualizer is drawing the overlay, in which
    /// case the engine leaves the frame alone instead of drawing a second overlay over it
    int32_t pipelineDrawsOverlay;

    /// @brief Bumped whenever the graphics device was lost and rebuilt. The swap chains of
    /// an earlier generation are dead, so a host that sees this change has to fetch them
    /// again and rebind its panels.
    int32_t rendererGeneration;

    /// @brief Bumped whenever the set of profiler stages, or their order, changes. The
    /// stages are keyed by index here and named through FeEngine_GetStageName, and the
    /// profiler discovers them as they first run - so a host that read the names once would
    /// keep showing them against the wrong timings. This is how it knows to read them again.
    int32_t stageLayoutVersion;

    FeFace faces[FE_MAX_FACES];
  } FeSnapshot;

  /// @brief What the video view draws on top of the frame. All flags are 0 or 1.
  typedef struct FeOverlayOptions
  {
    /// @brief Strength of the halo behind the mesh, 0 disables the pass entirely
    double glowStrength;

    /// @brief Radius of the landmark dots in pixels of the view
    double pointSize;

    int32_t showMesh;
    int32_t showPoints;
    int32_t showRect;
    int32_t showPoseBox;
    int32_t showAxes;
    int32_t showLabels;

    /// @brief Mirrors the view horizontally, which is what a user expects of a front camera
    int32_t mirror;

    int32_t reserved;
  } FeOverlayOptions;

  /// @brief What the head view draws and from where.
  typedef struct FeHeadOptions
  {
    /// @brief Orbit of the viewer around the head, in degrees, applied on top of the pose
    double orbitYawDeg;
    double orbitPitchDeg;

    /// @brief Camera distance as a multiple of the head radius
    double distance;

    /// @brief 0 keeps the head upright, 1 lets it follow the measured pose
    double poseFollow;

    /// @brief How strongly the landmarks deform the head, 1 is life-like, 0 is a rest pose
    double expression;

    int32_t showWireframe;
    int32_t showLandmarks;
    int32_t showEyes;

    /// @brief Slowly turns the head when no face is being tracked
    int32_t idleSpin;
  } FeHeadOptions;

  /// @brief Reports the size of the structs above, so a host in another language can check
  /// its own declarations against them once, at startup, instead of discovering a mismatch
  /// as a field that reads nonsense.
  FE_API void FE_CALL FeEngine_GetStructSizes(int32_t* oSnapshotSize, int32_t* oFaceSize);

  /// @name Lifetime
  /// @{

  /// @brief Creates the engine and initialises the FaceApi pipeline from iWorkingDirectory.
  /// @param iWorkingDirectory Directory holding settings.json and the model files
  /// @param oEngine Receives the engine handle, released with FeEngine_Destroy
  FE_API FeStatus FE_CALL FeEngine_Create(const wchar_t* iWorkingDirectory, void** oEngine);

  FE_API void FE_CALL FeEngine_Destroy(void* iEngine);

  /// @brief The message of the last call that failed, for the status line of the UI.
  FE_API FeStatus FE_CALL FeEngine_GetLastError(void* iEngine, wchar_t* oBuffer, int32_t iCapacity);

  /// @}
  /// @name Source
  /// @{

  /// @brief Probes capture device indices and caches what it found.
  /// @return The number of devices that could be opened, negative on failure
  FE_API int32_t FE_CALL FeEngine_EnumerateCameras(void* iEngine, int32_t iMaxProbe);

  FE_API FeStatus FE_CALL FeEngine_GetCameraName(void* iEngine, int32_t iIndex, wchar_t* oBuffer, int32_t iCapacity);

  /// @param iWidth, iHeight Requested capture size, 0 leaves the driver's default
  FE_API FeStatus FE_CALL FeEngine_OpenCamera(void* iEngine, int32_t iIndex, int32_t iWidth, int32_t iHeight, double iFps);

  FE_API FeStatus FE_CALL FeEngine_OpenFile(void* iEngine, const wchar_t* iPath);

  FE_API FeStatus FE_CALL FeEngine_CloseSource(void* iEngine);

  FE_API void FE_CALL FeEngine_SetPaused(void* iEngine, int32_t iPaused);

  /// @brief Loops a file source back to its first frame. No effect on a camera.
  FE_API void FE_CALL FeEngine_SetLooping(void* iEngine, int32_t iLooping);

  /// @}
  /// @name Views
  /// @{

  /// @brief Creates the composition swap chain of the video view.
  /// @param oSwapChain Receives an IDXGISwapChain1* with a reference the caller owns. The
  /// managed side hands it to ISwapChainPanelNative::SetSwapChain and releases it.
  FE_API FeStatus FE_CALL FeEngine_CreateVideoSwapChain(void* iEngine, void** oSwapChain);

  /// @param iScaleX, iScaleY Panel-to-pixel scale, so the swap chain matches the DPI the
  /// panel is composited at rather than the logical size XAML reports
  FE_API FeStatus FE_CALL FeEngine_ResizeVideoView(void* iEngine, int32_t iWidth, int32_t iHeight, double iScaleX, double iScaleY);

  FE_API FeStatus FE_CALL FeEngine_CreateHeadSwapChain(void* iEngine, void** oSwapChain);

  FE_API FeStatus FE_CALL FeEngine_ResizeHeadView(void* iEngine, int32_t iWidth, int32_t iHeight, double iScaleX, double iScaleY);

  FE_API void FE_CALL FeEngine_SetOverlayOptions(void* iEngine, const FeOverlayOptions* iOptions);

  FE_API void FE_CALL FeEngine_SetHeadOptions(void* iEngine, const FeHeadOptions* iOptions);

  /// @brief Maps a point of the video view back to the source frame, for hit testing.
  FE_API FeStatus FE_CALL FeEngine_ViewToFrame(void* iEngine, double iViewX, double iViewY, double* oFrameX, double* oFrameY);

  /// @}
  /// @name Results and control
  /// @{

  FE_API FeStatus FE_CALL FeEngine_GetSnapshot(void* iEngine, FeSnapshot* oSnapshot);

  FE_API FeStatus FE_CALL FeEngine_GetStageName(void* iEngine, int32_t iIndex, wchar_t* oBuffer, int32_t iCapacity);

  /// @brief Tears the pipeline down and builds it again from settings.json, keeping the
  /// source open. This is what the settings editor calls after it saved.
  FE_API FeStatus FE_CALL FeEngine_ReloadPipeline(void* iEngine);

  FE_API void FE_CALL FeEngine_ClearUsers(void* iEngine);

  FE_API void FE_CALL FeEngine_ForceDetection(void* iEngine);

  FE_API void FE_CALL FeEngine_SetVerbose(void* iEngine, int32_t iVerbose);

  /// @brief Writes the frame as it is displayed, overlay included, to iPath as PNG.
  FE_API FeStatus FE_CALL FeEngine_SaveFrame(void* iEngine, const wchar_t* iPath);

  /// @brief Starts or stops writing the displayed frames to a video file in iDirectory.
  FE_API FeStatus FE_CALL FeEngine_SetRecording(void* iEngine, int32_t iRecording, const wchar_t* iDirectory);

  /// @}

#ifdef __cplusplus
}
#endif
