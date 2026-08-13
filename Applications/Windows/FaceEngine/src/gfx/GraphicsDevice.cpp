#include "gfx/GraphicsDevice.h"

namespace fe::gfx
{
  namespace
  {
    /// @brief Feature levels in the order they are worth asking for. 9_3 keeps the engine
    /// alive on a software adapter, which is what a remote session hands out.
    constexpr D3D_FEATURE_LEVEL sFeatureLevels[] = {
      D3D_FEATURE_LEVEL_11_1,
      D3D_FEATURE_LEVEL_11_0,
      D3D_FEATURE_LEVEL_10_1,
      D3D_FEATURE_LEVEL_10_0,
      D3D_FEATURE_LEVEL_9_3
    };
  }

  GraphicsDevice::~GraphicsDevice()
  {
    Destroy();
  }

  bool GraphicsDevice::Create()
  {
    Destroy();

    // BGRA is not optional here: Direct2D refuses a device created without it, and the
    // whole overlay is drawn through Direct2D.
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
    // Absent on a machine without the graphics tools optional feature, so the creation is
    // retried without it rather than failing the whole engine on a developer's box.
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;

    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
                                   sFeatureLevels, ARRAYSIZE(sFeatureLevels), D3D11_SDK_VERSION,
                                   &device, nullptr, &context);

#if defined(_DEBUG)
    if (FAILED(hr))
    {
      flags &= ~static_cast<UINT>(D3D11_CREATE_DEVICE_DEBUG);
      hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
                             sFeatureLevels, ARRAYSIZE(sFeatureLevels), D3D11_SDK_VERSION,
                             &device, nullptr, &context);
    }
#endif

    if (FAILED(hr))
    {
      // No usable GPU, which a virtual machine or a locked-down session can produce
      hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, flags,
                             sFeatureLevels, ARRAYSIZE(sFeatureLevels), D3D11_SDK_VERSION,
                             &device, nullptr, &context);
    }

    if (FAILED(hr)) return false;

    if (FAILED(device.As(&mDevice)) || FAILED(context.As(&mContext)))
    {
      Destroy();
      return false;
    }

    ComPtr<IDXGIDevice1> dxgiDevice;
    if (FAILED(mDevice.As(&dxgiDevice)))
    {
      Destroy();
      return false;
    }

    // One frame of latency instead of the default three: the head and the overlay are
    // reactions to what the user just did, and three frames of queued presents read as lag.
    dxgiDevice->SetMaximumFrameLatency(1U);

    ComPtr<IDXGIAdapter> adapter;
    if (FAILED(dxgiDevice->GetAdapter(&adapter)) ||
        FAILED(adapter->GetParent(IID_PPV_ARGS(&mDxgiFactory))))
    {
      Destroy();
      return false;
    }

    D2D1_FACTORY_OPTIONS d2dOptions{};
#if defined(_DEBUG)
    d2dOptions.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, __uuidof(ID2D1Factory1),
                                 &d2dOptions, reinterpret_cast<void**>(mD2DFactory.GetAddressOf()))))
    {
      d2dOptions.debugLevel = D2D1_DEBUG_LEVEL_NONE;

      if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, __uuidof(ID2D1Factory1),
                                   &d2dOptions, reinterpret_cast<void**>(mD2DFactory.GetAddressOf()))))
      {
        Destroy();
        return false;
      }
    }

    if (FAILED(mD2DFactory->CreateDevice(dxgiDevice.Get(), &mD2DDevice)))
    {
      Destroy();
      return false;
    }

    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory1),
                                   reinterpret_cast<IUnknown**>(mWriteFactory.GetAddressOf()))))
    {
      Destroy();
      return false;
    }

    return true;
  }

  void GraphicsDevice::Destroy()
  {
    if (mContext)
    {
      mContext->ClearState();
      mContext->Flush();
    }

    mWriteFactory.Reset();
    mD2DDevice.Reset();
    mD2DFactory.Reset();
    mDxgiFactory.Reset();
    mContext.Reset();
    mDevice.Reset();
  }

  bool GraphicsDevice::Recreate()
  {
    Destroy();

    if (!Create()) return false;

    ++mGeneration;
    return true;
  }

  bool GraphicsDevice::CreateCompositionSwapChain(uint32_t iWidth, uint32_t iHeight, IDXGISwapChain1** oSwapChain) const
  {
    if (!oSwapChain) return false;
    *oSwapChain = nullptr;

    if (!mDevice || !mDxgiFactory) return false;

    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.Width = iWidth > 0U ? iWidth : 1U;
    desc.Height = iHeight > 0U ? iHeight : 1U;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.Stereo = FALSE;
    desc.SampleDesc.Count = 1U;
    desc.SampleDesc.Quality = 0U;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2U;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

    // A SwapChainPanel composites the chain into the XAML tree, so what is not covered has
    // to stay see-through rather than come out black
    desc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

    return SUCCEEDED(mDxgiFactory->CreateSwapChainForComposition(mDevice.Get(), &desc, nullptr, oSwapChain));
  }

  bool GraphicsDevice::IsDeviceLost(HRESULT iResult)
  {
    return iResult == DXGI_ERROR_DEVICE_REMOVED ||
           iResult == DXGI_ERROR_DEVICE_RESET ||
           iResult == DXGI_ERROR_DEVICE_HUNG ||
           iResult == DXGI_ERROR_DRIVER_INTERNAL_ERROR;
  }
}
