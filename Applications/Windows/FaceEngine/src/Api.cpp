/// @file Api.cpp
/// @brief The flat interface declared in FaceEngine.h, over fe::Engine.
///
/// Nothing here does any work. Its whole job is to keep the boundary narrow: handles instead
/// of objects, buffers instead of strings, and no exception ever crossing out of the DLL,
/// because there is no C# side of an exception that unwound through a P/Invoke.

#include "FaceEngine.h"

#include "Engine.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>

namespace
{
  fe::Engine* AsEngine(void* iEngine)
  {
    return static_cast<fe::Engine*>(iEngine);
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

  FE_API FeStatus FE_CALL FeEngine_Create(const wchar_t* iWorkingDirectory, void** oEngine)
  {
    if (!oEngine) return FeStatus_InvalidArgument;

    *oEngine = nullptr;

    try
    {
      auto engine = std::make_unique<fe::Engine>();

      if (!engine->Initialize(iWorkingDirectory ? iWorkingDirectory : L""))
      {
        // Handed back anyway: the reason it failed is on it, and the host needs to be able
        // to read it before it lets go
        *oEngine = engine.release();
        return FeStatus_NotInitialized;
      }

      *oEngine = engine.release();
      return FeStatus_Ok;
    }
    catch (...)
    {
      return FeStatus_Failed;
    }
  }

  FE_API void FE_CALL FeEngine_Destroy(void* iEngine)
  {
    try
    {
      delete AsEngine(iEngine);
    }
    catch (...)
    {
      // A destructor that threw has nowhere to report it, and taking down the host over it
      // would be worse than the leak
    }
  }

  FE_API FeStatus FE_CALL FeEngine_GetLastError(void* iEngine, wchar_t* oBuffer, int32_t iCapacity)
  {
    if (!iEngine) return FeStatus_InvalidArgument;

    try
    {
      return CopyString(AsEngine(iEngine)->GetLastError(), oBuffer, iCapacity);
    }
    catch (...)
    {
      return FeStatus_Failed;
    }
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

    try
    {
      std::wstring name;

      if (!AsEngine(iEngine)->GetCameraName(iIndex, name)) return FeStatus_NoData;

      return CopyString(name, oBuffer, iCapacity);
    }
    catch (...)
    {
      return FeStatus_Failed;
    }
  }

  FE_API FeStatus FE_CALL FeEngine_OpenCamera(void* iEngine, int32_t iIndex, int32_t iWidth, int32_t iHeight, double iFps)
  {
    if (!iEngine) return FeStatus_InvalidArgument;

    try
    {
      return AsEngine(iEngine)->OpenCamera(iIndex, iWidth, iHeight, iFps) ? FeStatus_Ok : FeStatus_SourceFailed;
    }
    catch (...)
    {
      return FeStatus_Failed;
    }
  }

  FE_API FeStatus FE_CALL FeEngine_OpenFile(void* iEngine, const wchar_t* iPath)
  {
    if (!iEngine || !iPath) return FeStatus_InvalidArgument;

    try
    {
      return AsEngine(iEngine)->OpenFile(iPath) ? FeStatus_Ok : FeStatus_SourceFailed;
    }
    catch (...)
    {
      return FeStatus_Failed;
    }
  }

  FE_API FeStatus FE_CALL FeEngine_CloseSource(void* iEngine)
  {
    if (!iEngine) return FeStatus_InvalidArgument;

    try
    {
      AsEngine(iEngine)->CloseSource();
      return FeStatus_Ok;
    }
    catch (...)
    {
      return FeStatus_Failed;
    }
  }

  FE_API void FE_CALL FeEngine_SetPaused(void* iEngine, int32_t iPaused)
  {
    if (!iEngine) return;

    try
    {
      AsEngine(iEngine)->SetPaused(iPaused != 0);
    }
    catch (...)
    {
    }
  }

  FE_API void FE_CALL FeEngine_SetLooping(void* iEngine, int32_t iLooping)
  {
    if (!iEngine) return;

    try
    {
      AsEngine(iEngine)->SetLooping(iLooping != 0);
    }
    catch (...)
    {
    }
  }

  FE_API FeStatus FE_CALL FeEngine_CreateVideoSwapChain(void* iEngine, void** oSwapChain)
  {
    if (!iEngine || !oSwapChain) return FeStatus_InvalidArgument;

    *oSwapChain = nullptr;

    try
    {
      IDXGISwapChain1* chain = nullptr;

      if (!AsEngine(iEngine)->CreateVideoSwapChain(&chain)) return FeStatus_Failed;

      *oSwapChain = chain;
      return FeStatus_Ok;
    }
    catch (...)
    {
      return FeStatus_Failed;
    }
  }

  FE_API FeStatus FE_CALL FeEngine_ResizeVideoView(void* iEngine, int32_t iWidth, int32_t iHeight, double iScaleX, double iScaleY)
  {
    if (!iEngine) return FeStatus_InvalidArgument;

    try
    {
      return AsEngine(iEngine)->ResizeVideoView(iWidth, iHeight, iScaleX, iScaleY) ? FeStatus_Ok : FeStatus_Failed;
    }
    catch (...)
    {
      return FeStatus_Failed;
    }
  }

  FE_API FeStatus FE_CALL FeEngine_CreateHeadSwapChain(void* iEngine, void** oSwapChain)
  {
    if (!iEngine || !oSwapChain) return FeStatus_InvalidArgument;

    *oSwapChain = nullptr;

    try
    {
      IDXGISwapChain1* chain = nullptr;

      if (!AsEngine(iEngine)->CreateHeadSwapChain(&chain)) return FeStatus_Failed;

      *oSwapChain = chain;
      return FeStatus_Ok;
    }
    catch (...)
    {
      return FeStatus_Failed;
    }
  }

  FE_API FeStatus FE_CALL FeEngine_ResizeHeadView(void* iEngine, int32_t iWidth, int32_t iHeight, double iScaleX, double iScaleY)
  {
    if (!iEngine) return FeStatus_InvalidArgument;

    try
    {
      return AsEngine(iEngine)->ResizeHeadView(iWidth, iHeight, iScaleX, iScaleY) ? FeStatus_Ok : FeStatus_Failed;
    }
    catch (...)
    {
      return FeStatus_Failed;
    }
  }

  FE_API void FE_CALL FeEngine_SetOverlayOptions(void* iEngine, const FeOverlayOptions* iOptions)
  {
    if (!iEngine || !iOptions) return;

    try
    {
      AsEngine(iEngine)->SetOverlayOptions(*iOptions);
    }
    catch (...)
    {
    }
  }

  FE_API void FE_CALL FeEngine_SetHeadOptions(void* iEngine, const FeHeadOptions* iOptions)
  {
    if (!iEngine || !iOptions) return;

    try
    {
      AsEngine(iEngine)->SetHeadOptions(*iOptions);
    }
    catch (...)
    {
    }
  }

  FE_API FeStatus FE_CALL FeEngine_ViewToFrame(void* iEngine, double iViewX, double iViewY, double* oFrameX, double* oFrameY)
  {
    if (!iEngine || !oFrameX || !oFrameY) return FeStatus_InvalidArgument;

    try
    {
      return AsEngine(iEngine)->ViewToFrame(iViewX, iViewY, *oFrameX, *oFrameY) ? FeStatus_Ok : FeStatus_NoData;
    }
    catch (...)
    {
      return FeStatus_Failed;
    }
  }

  FE_API FeStatus FE_CALL FeEngine_GetSnapshot(void* iEngine, FeSnapshot* oSnapshot)
  {
    if (!iEngine || !oSnapshot) return FeStatus_InvalidArgument;

    try
    {
      AsEngine(iEngine)->GetSnapshot(*oSnapshot);
      return FeStatus_Ok;
    }
    catch (...)
    {
      return FeStatus_Failed;
    }
  }

  FE_API FeStatus FE_CALL FeEngine_GetStageName(void* iEngine, int32_t iIndex, wchar_t* oBuffer, int32_t iCapacity)
  {
    if (!iEngine) return FeStatus_InvalidArgument;

    try
    {
      std::wstring name;

      if (!AsEngine(iEngine)->GetStageName(iIndex, name)) return FeStatus_NoData;

      return CopyString(name, oBuffer, iCapacity);
    }
    catch (...)
    {
      return FeStatus_Failed;
    }
  }

  FE_API FeStatus FE_CALL FeEngine_ReloadPipeline(void* iEngine)
  {
    if (!iEngine) return FeStatus_InvalidArgument;

    try
    {
      return AsEngine(iEngine)->ReloadPipeline() ? FeStatus_Ok : FeStatus_NotInitialized;
    }
    catch (...)
    {
      return FeStatus_Failed;
    }
  }

  FE_API void FE_CALL FeEngine_ClearUsers(void* iEngine)
  {
    if (!iEngine) return;

    try
    {
      AsEngine(iEngine)->ClearUsers();
    }
    catch (...)
    {
    }
  }

  FE_API void FE_CALL FeEngine_ForceDetection(void* iEngine)
  {
    if (!iEngine) return;

    try
    {
      AsEngine(iEngine)->ForceDetection();
    }
    catch (...)
    {
    }
  }

  FE_API void FE_CALL FeEngine_SetVerbose(void* iEngine, int32_t iVerbose)
  {
    if (!iEngine) return;

    try
    {
      AsEngine(iEngine)->SetVerbose(iVerbose != 0);
    }
    catch (...)
    {
    }
  }

  FE_API FeStatus FE_CALL FeEngine_SaveFrame(void* iEngine, const wchar_t* iPath)
  {
    if (!iEngine || !iPath) return FeStatus_InvalidArgument;

    try
    {
      return AsEngine(iEngine)->SaveFrame(iPath) ? FeStatus_Ok : FeStatus_Failed;
    }
    catch (...)
    {
      return FeStatus_Failed;
    }
  }

  FE_API FeStatus FE_CALL FeEngine_SetRecording(void* iEngine, int32_t iRecording, const wchar_t* iDirectory)
  {
    if (!iEngine) return FeStatus_InvalidArgument;

    try
    {
      return AsEngine(iEngine)->SetRecording(iRecording != 0, iDirectory ? iDirectory : L"")
               ? FeStatus_Ok
               : FeStatus_Failed;
    }
    catch (...)
    {
      return FeStatus_Failed;
    }
  }
}
