#pragma once

#include "FaceEngine.h"
#include "gfx/GraphicsDevice.h"

#include "Model/HeadMesh.h"
#include "ViewFrame.h"

#include <cstdint>
#include <vector>

namespace fe::gfx
{
  /// @brief The artificial head, rendered with Direct3D.
  ///
  /// The head itself - what its surface is and where the landmarks move it - is
  /// face::HeadMesh, which knows nothing about any graphics API. This class owns the part
  /// that is Direct3D: the buffers, the shaders, the camera and the multisampled target the
  /// silhouette is resolved from.
  class HeadView
  {
  public:
    explicit HeadView(GraphicsDevice& ioDevice);

    HeadView(const HeadView& iOther) = delete;

    ~HeadView();

    HeadView& operator=(const HeadView& iOther) = delete;

    bool Create(uint32_t iWidth, uint32_t iHeight);

    void Destroy();

    inline IDXGISwapChain1* GetSwapChain() const
    {
      return mSwapChain.Get();
    }

    inline bool IsValid() const
    {
      return mSwapChain != nullptr;
    }

    bool Resize(uint32_t iWidth, uint32_t iHeight, double iScaleX, double iScaleY);

    void SetOptions(const FeHeadOptions& iOptions);

    /// @brief Drives the head from the first face of iFrame and presents.
    /// @param iElapsedSeconds Time since the last call, for the idle animation
    /// @return false when the device was lost and has to be rebuilt
    bool Render(const ViewFramePtr& iFrame, double iElapsedSeconds);

  private:
    /// @brief What a vertex looks like to the shader: position, normal, and the material and
    /// the baked occlusion packed into one pair.
    struct ShaderVertex
    {
      float position[3];
      float normal[3];
      float material;
      float occlusion;
    };

    struct Constants
    {
      float worldViewProjection[16];
      float world[16];
      float cameraPosition[4];
      float lightDirection[4];
      float accent[4];
      float outline[4];
      float params[4];
    };

    bool CreateTargets();

    void ReleaseTargets();

    bool CreateShaders();

    bool CreateGeometry();

    /// @brief Uploads the deformed mesh into the dynamic vertex buffer
    bool UploadMesh();

    /// @brief Fills the marker buffer with a small solid at each landmark
    void BuildLandmarkMarkers();

    void UpdateConstants(const face::FaceResult* iFace, double iElapsedSeconds, bool iWireframePass);

    static void ComputeAccent(int iUserId, float oAccent[4]);

    GraphicsDevice& mDevice;

    ComPtr<IDXGISwapChain1> mSwapChain;

    /// @brief Multisampled colour and depth, resolved into the back buffer on present. A
    /// head against a transparent panel is nearly all silhouette, so its edges are what the
    /// view is judged by.
    ComPtr<ID3D11Texture2D> mColorBuffer;
    ComPtr<ID3D11RenderTargetView> mColorView;
    ComPtr<ID3D11Texture2D> mDepthBuffer;
    ComPtr<ID3D11DepthStencilView> mDepthView;

    ComPtr<ID3D11VertexShader> mVertexShader;
    ComPtr<ID3D11PixelShader> mPixelShader;
    ComPtr<ID3D11InputLayout> mInputLayout;

    ComPtr<ID3D11Buffer> mVertexBuffer;
    ComPtr<ID3D11Buffer> mIndexBuffer;
    ComPtr<ID3D11Buffer> mEdgeIndexBuffer;
    ComPtr<ID3D11Buffer> mMarkerVertexBuffer;
    ComPtr<ID3D11Buffer> mMarkerIndexBuffer;
    ComPtr<ID3D11Buffer> mConstantBuffer;

    ComPtr<ID3D11RasterizerState> mRasterizer;
    ComPtr<ID3D11DepthStencilState> mDepthState;
    ComPtr<ID3D11BlendState> mBlendState;

    face::HeadMesh mMesh;

    /// @brief Staging for the upload, kept so that a frame does not allocate
    std::vector<ShaderVertex> mShaderVertices;
    std::vector<ShaderVertex> mMarkerVertices;

    uint32_t mIndexCount = 0U;
    uint32_t mEdgeIndexCount = 0U;
    uint32_t mMarkerIndexCount = 0U;

    uint32_t mWidth = 1U;
    uint32_t mHeight = 1U;
    uint32_t mSampleCount = 1U;

    /// @brief Turns slowly while nothing is being tracked, so the view is never quite still
    double mIdleAngle = 0.0;

    /// @brief The frame the mesh was last driven with, so a shape is not smoothed twice
    uint32_t mLastDrivenFrameId = 0U;

    FeHeadOptions mOptions{};
  };
}
