/// @file Api.cpp
/// @brief The flat interface declared in FaceEngine.h, over fe::Engine.
///
/// Nothing here does any work. Its whole job is to keep the boundary narrow: handles instead
/// of objects, buffers instead of strings, and no exception ever crossing out of the DLL,
/// because there is no C# side of an exception that unwound through a P/Invoke.

#include "FaceEngine.h"

#include "Engine.h"

#include "Model/Palette.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <utility>

namespace
{
  fe::Engine* AsEngine(void* iEngine)
  {
    return static_cast<fe::Engine*>(iEngine);
  }

  /// @brief Runs iBody and turns anything it throws into iOnError - the one job every
  /// function of this boundary shares, written once instead of per function.
  template <typename BodyT>
  FeStatus Guarded(FeStatus iOnError, BodyT&& iBody)
  {
    try
    {
      return std::forward<BodyT>(iBody)();
    }
    catch (...)
    {
      return iOnError;
    }
  }

  /// @brief The void flavour: the call has nothing to report, so a failure is swallowed.
  template <typename BodyT>
  void GuardedVoid(BodyT&& iBody)
  {
    try
    {
      std::forward<BodyT>(iBody)();
    }
    catch (...)
    {
    }
  }

  /// @brief Copies iText into the caller's buffer, always terminated, never overrun.
  FeStatus CopyString(const std::wstring& iText, wchar_t* oBuffer, int32_t iCapacity)
  {
    if (!oBuffer || iCapacity <= 0) return FeStatus_InvalidArgument;

    const std::size_t room = static_cast<std::size_t>(iCapacity) - 1U;
    const std::size_t length = (std::min)(iText.size(), room);

    if (length > 0U) std::memcpy(oBuffer, iText.c_str(), length * sizeof(wchar_t));

    oBuffer[length] = L'\0';

    // The caller is told the text did not fit rather than being handed a silent truncation
    return length < iText.size() ? FeStatus_Failed : FeStatus_Ok;
  }
}

extern "C"
{
  FE_API void FE_CALL FeEngine_GetStructSizes(int32_t* oSnapshotSize, int32_t* oFaceSize)
  {
    if (oSnapshotSize) *oSnapshotSize = static_cast<int32_t>(sizeof(FeSnapshot));
    if (oFaceSize) *oFaceSize = static_cast<int32_t>(sizeof(FeFace));
  }

  FE_API int32_t FE_CALL FeEngine_GetAccentCount(void)
  {
    return static_cast<int32_t>(face::palette::kAccents.size());
  }

  FE_API void FE_CALL FeEngine_GetAccentColor(int32_t iIndex, double* oR, double* oG, double* oB)
  {
    const face::palette::Rgb& color = face::palette::accent_of(iIndex);

    if (oR) *oR = color.r;
    if (oG) *oG = color.g;
    if (oB) *oB = color.b;
  }

  FE_API FeStatus FE_CALL FeEngine_Create(const wchar_t* iWorkingDirectory, void** oEngine)
  {
    if (!oEngine) return FeStatus_InvalidArgument;

    *oEngine = nullptr;

    return Guarded(FeStatus_Failed, [&] {
      auto engine = std::make_unique<fe::Engine>();

      // Handed back even on failure: the reason it failed is on it, and the host needs to
      // be able to read it before it lets go
      const bool initialized = engine->Initialize(iWorkingDirectory ? iWorkingDirectory : L"");
      *oEngine = engine.release();

      return initialized ? FeStatus_Ok : FeStatus_NotInitialized;
    });
  }

  FE_API void FE_CALL FeEngine_Destroy(void* iEngine)
  {
    // A destructor that threw has nowhere to report it, and taking down the host over it
    // would be worse than the leak
    GuardedVoid([&] { delete AsEngine(iEngine); });
  }

  FE_API FeStatus FE_CALL FeEngine_GetLastError(void* iEngine, wchar_t* oBuffer, int32_t iCapacity)
  {
    if (!iEngine) return FeStatus_InvalidArgument;

    return Guarded(FeStatus_Failed, [&] {
      return CopyString(AsEngine(iEngine)->GetLastError(), oBuffer, iCapacity);
    });
  }

  FE_API int32_t FE_CALL FeEngine_EnumerateCameras(void* iEngine, int32_t iMaxProbe)
  {
    if (!iEngine) return -1;

    try
    {
      return AsEngine(iEngine)->EnumerateCameras(iMaxProbe);
    }
    catch (...)
    {
      return -1;
    }
  }

  FE_API FeStatus FE_CALL FeEngine_GetCameraName(void* iEngine, int32_t iIndex, wchar_t* oBuffer, int32_t iCapacity)
  {
    if (!iEngine) return FeStatus_InvalidArgument;

    return Guarded(FeStatus_Failed, [&] {
      std::wstring name;

      if (!AsEngine(iEngine)->GetCameraName(iIndex, name)) return FeStatus_NoData;

      return CopyString(name, oBuffer, iCapacity);
    });
  }

  FE_API FeStatus FE_CALL FeEngine_OpenCamera(void* iEngine, int32_t iIndex, int32_t iWidth, int32_t iHeight, double iFps)
  {
    if (!iEngine) return FeStatus_InvalidArgument;

    return Guarded(FeStatus_Failed, [&] {
      return AsEngine(iEngine)->OpenCamera(iIndex, iWidth, iHeight, iFps) ? FeStatus_Ok : FeStatus_SourceFailed;
    });
  }

  FE_API FeStatus FE_CALL FeEngine_OpenFile(void* iEngine, const wchar_t* iPath)
  {
    if (!iEngine || !iPath) return FeStatus_InvalidArgument;

    return Guarded(FeStatus_Failed, [&] {
      return AsEngine(iEngine)->OpenFile(iPath) ? FeStatus_Ok : FeStatus_SourceFailed;
    });
  }

  FE_API FeStatus FE_CALL FeEngine_CloseSource(void* iEngine)
  {
    if (!iEngine) return FeStatus_InvalidArgument;

    return Guarded(FeStatus_Failed, [&] {
      AsEngine(iEngine)->CloseSource();
      return FeStatus_Ok;
    });
  }

  FE_API void FE_CALL FeEngine_SetPaused(void* iEngine, int32_t iPaused)
  {
    if (!iEngine) return;

    GuardedVoid([&] { AsEngine(iEngine)->SetPaused(iPaused != 0); });
  }

  FE_API void FE_CALL FeEngine_SetLooping(void* iEngine, int32_t iLooping)
  {
    if (!iEngine) return;

    GuardedVoid([&] { AsEngine(iEngine)->SetLooping(iLooping != 0); });
  }

  FE_API FeStatus FE_CALL FeEngine_CreateVideoSwapChain(void* iEngine, void** oSwapChain)
  {
    if (!iEngine || !oSwapChain) return FeStatus_InvalidArgument;

    *oSwapChain = nullptr;

    return Guarded(FeStatus_Failed, [&] {
      IDXGISwapChain1* chain = nullptr;

      if (!AsEngine(iEngine)->CreateVideoSwapChain(&chain)) return FeStatus_Failed;

      *oSwapChain = chain;
      return FeStatus_Ok;
    });
  }

  FE_API FeStatus FE_CALL FeEngine_ResizeVideoView(void* iEngine, int32_t iWidth, int32_t iHeight, double iScaleX, double iScaleY)
  {
    if (!iEngine) return FeStatus_InvalidArgument;

    return Guarded(FeStatus_Failed, [&] {
      return AsEngine(iEngine)->ResizeVideoView(iWidth, iHeight, iScaleX, iScaleY) ? FeStatus_Ok : FeStatus_Failed;
    });
  }

  FE_API FeStatus FE_CALL FeEngine_CreateHeadSwapChain(void* iEngine, void** oSwapChain)
  {
    if (!iEngine || !oSwapChain) return FeStatus_InvalidArgument;

    *oSwapChain = nullptr;

    return Guarded(FeStatus_Failed, [&] {
      IDXGISwapChain1* chain = nullptr;

      if (!AsEngine(iEngine)->CreateHeadSwapChain(&chain)) return FeStatus_Failed;

      *oSwapChain = chain;
      return FeStatus_Ok;
    });
  }

  FE_API FeStatus FE_CALL FeEngine_ResizeHeadView(void* iEngine, int32_t iWidth, int32_t iHeight, double iScaleX, double iScaleY)
  {
    if (!iEngine) return FeStatus_InvalidArgument;

    return Guarded(FeStatus_Failed, [&] {
      return AsEngine(iEngine)->ResizeHeadView(iWidth, iHeight, iScaleX, iScaleY) ? FeStatus_Ok : FeStatus_Failed;
    });
  }

  FE_API void FE_CALL FeEngine_SetOverlayOptions(void* iEngine, const FeOverlayOptions* iOptions)
  {
    if (!iEngine || !iOptions) return;

    GuardedVoid([&] { AsEngine(iEngine)->SetOverlayOptions(*iOptions); });
  }

  FE_API void FE_CALL FeEngine_SetHeadOptions(void* iEngine, const FeHeadOptions* iOptions)
  {
    if (!iEngine || !iOptions) return;

    GuardedVoid([&] { AsEngine(iEngine)->SetHeadOptions(*iOptions); });
  }

  FE_API FeStatus FE_CALL FeEngine_ViewToFrame(void* iEngine, double iViewX, double iViewY, double* oFrameX, double* oFrameY)
  {
    if (!iEngine || !oFrameX || !oFrameY) return FeStatus_InvalidArgument;

    return Guarded(FeStatus_Failed, [&] {
      return AsEngine(iEngine)->ViewToFrame(iViewX, iViewY, *oFrameX, *oFrameY) ? FeStatus_Ok : FeStatus_NoData;
    });
  }

  FE_API FeStatus FE_CALL FeEngine_GetSnapshot(void* iEngine, FeSnapshot* oSnapshot)
  {
    if (!iEngine || !oSnapshot) return FeStatus_InvalidArgument;

    return Guarded(FeStatus_Failed, [&] {
      AsEngine(iEngine)->GetSnapshot(*oSnapshot);
      return FeStatus_Ok;
    });
  }

  FE_API FeStatus FE_CALL FeEngine_GetStageName(void* iEngine, int32_t iIndex, wchar_t* oBuffer, int32_t iCapacity)
  {
    if (!iEngine) return FeStatus_InvalidArgument;

    return Guarded(FeStatus_Failed, [&] {
      std::wstring name;

      if (!AsEngine(iEngine)->GetStageName(iIndex, name)) return FeStatus_NoData;

      return CopyString(name, oBuffer, iCapacity);
    });
  }

  FE_API FeStatus FE_CALL FeEngine_ReloadPipeline(void* iEngine)
  {
    if (!iEngine) return FeStatus_InvalidArgument;

    return Guarded(FeStatus_Failed, [&] {
      return AsEngine(iEngine)->ReloadPipeline() ? FeStatus_Ok : FeStatus_NotInitialized;
    });
  }

  FE_API void FE_CALL FeEngine_ClearUsers(void* iEngine)
  {
    if (!iEngine) return;

    GuardedVoid([&] { AsEngine(iEngine)->ClearUsers(); });
  }

  FE_API void FE_CALL FeEngine_ForceDetection(void* iEngine)
  {
    if (!iEngine) return;

    GuardedVoid([&] { AsEngine(iEngine)->ForceDetection(); });
  }

  FE_API void FE_CALL FeEngine_SetVerbose(void* iEngine, int32_t iVerbose)
  {
    if (!iEngine) return;

    GuardedVoid([&] { AsEngine(iEngine)->SetVerbose(iVerbose != 0); });
  }

  FE_API FeStatus FE_CALL FeEngine_SaveFrame(void* iEngine, const wchar_t* iPath)
  {
    if (!iEngine || !iPath) return FeStatus_InvalidArgument;

    return Guarded(FeStatus_Failed, [&] {
      return AsEngine(iEngine)->SaveFrame(iPath) ? FeStatus_Ok : FeStatus_Failed;
    });
  }

  FE_API FeStatus FE_CALL FeEngine_SetRecording(void* iEngine, int32_t iRecording, const wchar_t* iDirectory)
  {
    if (!iEngine) return FeStatus_InvalidArgument;

    return Guarded(FeStatus_Failed, [&] {
      return AsEngine(iEngine)->SetRecording(iRecording != 0, iDirectory ? iDirectory : L"")
               ? FeStatus_Ok
               : FeStatus_Failed;
    });
  }
}
