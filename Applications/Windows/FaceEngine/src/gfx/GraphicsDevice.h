#pragma once

#include <d2d1_1.h>
#include <d3d11_1.h>
#include <dwrite_1.h>
#include <dxgi1_3.h>
#include <wrl/client.h>

#include <cstdint>

namespace fe::gfx
{
  template <typename T>
  using ComPtr = Microsoft::WRL::ComPtr<T>;

  /// @brief The one Direct3D device both views draw with, plus the Direct2D and DirectWrite
  /// objects layered on it.
  ///
  /// Sharing a device is what lets the video view and the head view be presented from the
  /// same thread without a copy between them, and it is why a device loss has to be handled
  /// in one place: Recreate() rebuilds it, and everything that held resources of the old one
  /// is expected to rebuild itself against the new generation.
  class GraphicsDevice
  {
  public:
    GraphicsDevice() = default;

    GraphicsDevice(const GraphicsDevice& iOther) = delete;

    ~GraphicsDevice();

    GraphicsDevice& operator=(const GraphicsDevice& iOther) = delete;

    bool Create();

    void Destroy();

    /// @brief Tears the device down and builds a new one, bumping the generation.
    bool Recreate();

    inline bool IsValid() const
    {
      return mDevice != nullptr;
    }

    /// @brief Incremented by every successful Recreate(), so a holder of a swap chain can
    /// tell that what it is holding belongs to a device that no longer exists.
    inline uint32_t GetGeneration() const
    {
      return mGeneration;
    }

    inline ID3D11Device1* GetD3DDevice() const
    {
      return mDevice.Get();
    }

    inline ID3D11DeviceContext1* GetD3DContext() const
    {
      return mContext.Get();
    }

    inline IDXGIFactory2* GetDxgiFactory() const
    {
      return mDxgiFactory.Get();
    }

    inline ID2D1Device* GetD2DDevice() const
    {
      return mD2DDevice.Get();
    }

    inline IDWriteFactory1* GetWriteFactory() const
    {
      return mWriteFactory.Get();
    }

    /// @brief Creates a swap chain meant for an XAML SwapChainPanel: no window, composited
    /// by the visual tree, premultiplied so the panel can be laid over the page background.
    bool CreateCompositionSwapChain(uint32_t iWidth, uint32_t iHeight, IDXGISwapChain1** oSwapChain) const;

    /// @brief True when the result of a Present or a resource call means the device is gone
    /// and the caller should ask for a Recreate() rather than retry.
    static bool IsDeviceLost(HRESULT iResult);

  private:
    ComPtr<ID3D11Device1> mDevice;
    ComPtr<ID3D11DeviceContext1> mContext;
    ComPtr<IDXGIFactory2> mDxgiFactory;
    ComPtr<ID2D1Factory1> mD2DFactory;
    ComPtr<ID2D1Device> mD2DDevice;
    ComPtr<IDWriteFactory1> mWriteFactory;

    uint32_t mGeneration = 1U;
  };
}
