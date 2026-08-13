#include "gfx/HeadView.h"

#include "Configuration.h"

#include <d3dcompiler.h>
#include <DirectXMath.h>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace fe::gfx
{
  namespace
  {
    using namespace DirectX;

    constexpr float sDegToRad = 0.01745329251994329577F;

    /// @brief One source for both stages: they share the constant buffer and the varyings,
    /// and keeping them together is what stops the two from drifting apart.
    constexpr char sShaderSource[] = R"(
cbuffer Constants : register(b0)
{
  float4x4 gWorldViewProjection;
  float4x4 gWorld;
  float4   gCameraPosition;
  float4   gLightDirection;
  float4   gAccent;
  float4   gOutline;
  float4   gParams;          // x: wireframe pass, y: elapsed time
};

struct VSInput
{
  float3 position : POSITION;
  float3 normal   : NORMAL;
  float2 surface  : TEXCOORD0;   // x: material, y: baked occlusion
};

struct PSInput
{
  float4 clip     : SV_POSITION;
  float3 world    : TEXCOORD0;
  float3 normal   : TEXCOORD1;
  float2 surface  : TEXCOORD2;
};

PSInput VSMain(VSInput input)
{
  PSInput output;

  output.clip = mul(float4(input.position, 1.0f), gWorldViewProjection);
  output.world = mul(float4(input.position, 1.0f), gWorld).xyz;
  output.normal = mul(float4(input.normal, 0.0f), gWorld).xyz;
  output.surface = input.surface;

  return output;
}

float3 MaterialColour(float id)
{
  if (id < 0.5f) return float3(0.94f, 0.76f, 0.66f);   // skin, flat and warm
  if (id < 1.5f) return float3(1.00f, 1.00f, 1.00f);   // eye white
  if (id < 2.5f) return gAccent.rgb;                   // iris, tied to the user's colour
  if (id < 3.5f) return float3(0.05f, 0.05f, 0.07f);   // pupil
  if (id < 4.5f) return float3(0.34f, 0.11f, 0.15f);   // the inside of the mouth

  return gAccent.rgb;                                  // landmark markers
}

float4 PSMain(PSInput input) : SV_TARGET
{
  if (gParams.x > 0.5f) return float4(gAccent.rgb * 0.75f, 1.0f);

  float3 N = normalize(input.normal);
  float3 V = normalize(gCameraPosition.xyz - input.world);

  // The mesh is generated, not authored, so its winding is not guaranteed. Turning the
  // normal towards the viewer costs one branch and removes the whole class of bug.
  if (dot(N, V) < 0.0f) N = -N;

  float3 L = normalize(-gLightDirection.xyz);
  float  occlusion = saturate(input.surface.y);
  float3 base = MaterialColour(input.surface.x);

  // Three steps instead of a ramp. Quantising the light is what makes a surface read as
  // drawn rather than as lit, and it is the whole of the cartoon look.
  float lambert = dot(N, L) * 0.5f + 0.5f;
  float shade = 0.56f
              + 0.24f * smoothstep(0.46f, 0.54f, lambert)
              + 0.20f * smoothstep(0.70f, 0.78f, lambert);

  float3 colour = base * shade * occlusion;

  // One highlight, stepped like the shading but gentle: a hard one breaks into white
  // patches wherever the generated surface is not perfectly smooth
  float3 H = normalize(L + V);
  colour += smoothstep(0.80f, 0.96f, pow(saturate(dot(N, H)), 28.0f)) * 0.14f * occlusion;

  // The contour. A cartoon needs a dark edge, and on a closed surface the silhouette is
  // exactly where the normal turns away from the viewer - which costs no second pass and
  // does not depend on a winding this mesh cannot promise.
  float facing = smoothstep(0.0f, 0.32f, dot(N, V));
  colour = lerp(gOutline.rgb, colour, facing);

  // A rim in the user's own colour, inside the outline, so the head is recognisably the
  // same thing as the overlay on the frame
  colour += gAccent.rgb * pow(1.0f - saturate(dot(N, V)), 4.0f) * 0.40f * facing;

  return float4(colour, 1.0f);
}
)";

    ComPtr<ID3DBlob> CompileShader(const char* iEntryPoint, const char* iTarget)
    {
      UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;

#if defined(_DEBUG)
      flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

      ComPtr<ID3DBlob> code;
      ComPtr<ID3DBlob> errors;

      const HRESULT hr = D3DCompile(sShaderSource, sizeof(sShaderSource) - 1U, "HeadView.hlsl",
                                    nullptr, nullptr, iEntryPoint, iTarget, flags, 0U, &code, &errors);

      if (FAILED(hr))
      {
#if defined(_DEBUG)
        if (errors) OutputDebugStringA(static_cast<const char*>(errors->GetBufferPointer()));
#endif
        return nullptr;
      }

      return code;
    }

    void StoreMatrix(const XMMATRIX& iMatrix, float oTarget[16])
    {
      XMFLOAT4X4 stored;

      // HLSL reads a matrix column-major by default, and this one is built row-major
      XMStoreFloat4x4(&stored, XMMatrixTranspose(iMatrix));

      std::memcpy(oTarget, &stored, sizeof(stored));
    }
  }

  HeadView::HeadView(GraphicsDevice& ioDevice) :
    mDevice(ioDevice)
  {
    mOptions.orbitYawDeg = 0.0;
    mOptions.orbitPitchDeg = 0.0;
    mOptions.distance = 3.1;
    mOptions.poseFollow = 1.0;
    mOptions.expression = 1.2;
    mOptions.showWireframe = 0;
    mOptions.showLandmarks = 0;
    mOptions.showEyes = 1;
    mOptions.idleSpin = 1;
  }

  HeadView::~HeadView()
  {
    Destroy();
  }

  bool HeadView::Create(uint32_t iWidth, uint32_t iHeight)
  {
    Destroy();

    mWidth = (std::max)(1U, iWidth);
    mHeight = (std::max)(1U, iHeight);

    if (!mDevice.CreateCompositionSwapChain(mWidth, mHeight, &mSwapChain)) return false;

    // The head is an authored model, deployed with the other model files
    const std::string path = face::Configuration::GetInstance().GetDirectories().shapeModel +
                             "head/ict_neutral_head.obj";

    if (!mMesh.Build(path))
    {
      Destroy();
      return false;
    }

    if (!CreateShaders() || !CreateGeometry() || !CreateTargets())
    {
      Destroy();
      return false;
    }

    return true;
  }

  void HeadView::Destroy()
  {
    ReleaseTargets();

    mBlendState.Reset();
    mDepthState.Reset();
    mRasterizer.Reset();
    mConstantBuffer.Reset();
    mMarkerIndexBuffer.Reset();
    mMarkerVertexBuffer.Reset();
    mEdgeIndexBuffer.Reset();
    mIndexBuffer.Reset();
    mVertexBuffer.Reset();
    mInputLayout.Reset();
    mPixelShader.Reset();
    mVertexShader.Reset();
    mSwapChain.Reset();

    mIndexCount = 0U;
    mEdgeIndexCount = 0U;
    mMarkerIndexCount = 0U;
  }

  bool HeadView::CreateShaders()
  {
    ID3D11Device1* device = mDevice.GetD3DDevice();
    if (!device) return false;

    const ComPtr<ID3DBlob> vertexCode = CompileShader("VSMain", "vs_4_0");
    const ComPtr<ID3DBlob> pixelCode = CompileShader("PSMain", "ps_4_0");

    if (!vertexCode || !pixelCode) return false;

    if (FAILED(device->CreateVertexShader(vertexCode->GetBufferPointer(), vertexCode->GetBufferSize(),
                                          nullptr, &mVertexShader)))
      return false;

    if (FAILED(device->CreatePixelShader(pixelCode->GetBufferPointer(), pixelCode->GetBufferSize(),
                                         nullptr, &mPixelShader)))
      return false;

    const D3D11_INPUT_ELEMENT_DESC elements[] = {
      { "POSITION", 0U, DXGI_FORMAT_R32G32B32_FLOAT, 0U, 0U, D3D11_INPUT_PER_VERTEX_DATA, 0U },
      { "NORMAL", 0U, DXGI_FORMAT_R32G32B32_FLOAT, 0U, 12U, D3D11_INPUT_PER_VERTEX_DATA, 0U },
      { "TEXCOORD", 0U, DXGI_FORMAT_R32G32_FLOAT, 0U, 24U, D3D11_INPUT_PER_VERTEX_DATA, 0U }
    };

    if (FAILED(device->CreateInputLayout(elements, ARRAYSIZE(elements),
                                         vertexCode->GetBufferPointer(), vertexCode->GetBufferSize(),
                                         &mInputLayout)))
      return false;

    D3D11_BUFFER_DESC constantDesc{};
    constantDesc.ByteWidth = sizeof(Constants);
    constantDesc.Usage = D3D11_USAGE_DYNAMIC;
    constantDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constantDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    if (FAILED(device->CreateBuffer(&constantDesc, nullptr, &mConstantBuffer))) return false;

    D3D11_RASTERIZER_DESC rasterDesc{};
    rasterDesc.FillMode = D3D11_FILL_SOLID;

    // The mesh is closed but generated, so its winding is not relied on; the pixel shader
    // turns the normal towards the viewer instead
    rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.DepthClipEnable = TRUE;
    rasterDesc.MultisampleEnable = TRUE;
    rasterDesc.AntialiasedLineEnable = TRUE;

    if (FAILED(device->CreateRasterizerState(&rasterDesc, &mRasterizer))) return false;

    D3D11_DEPTH_STENCIL_DESC depthDesc{};
    depthDesc.DepthEnable = TRUE;
    depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthDesc.DepthFunc = D3D11_COMPARISON_LESS;

    if (FAILED(device->CreateDepthStencilState(&depthDesc, &mDepthState))) return false;

    D3D11_BLEND_DESC blendDesc{};
    blendDesc.RenderTarget[0].BlendEnable = FALSE;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    return SUCCEEDED(device->CreateBlendState(&blendDesc, &mBlendState));
  }

  bool HeadView::CreateGeometry()
  {
    ID3D11Device1* device = mDevice.GetD3DDevice();
    if (!device || !mMesh.IsBuilt()) return false;

    const auto& vertices = mMesh.GetVertices();
    const auto& indices = mMesh.GetIndices();
    const auto& edges = mMesh.GetEdgeIndices();

    mShaderVertices.resize(vertices.size());

    D3D11_BUFFER_DESC vertexDesc{};
    vertexDesc.ByteWidth = static_cast<UINT>(sizeof(ShaderVertex) * vertices.size());
    vertexDesc.Usage = D3D11_USAGE_DYNAMIC;
    vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    if (FAILED(device->CreateBuffer(&vertexDesc, nullptr, &mVertexBuffer))) return false;

    D3D11_BUFFER_DESC indexDesc{};
    indexDesc.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * indices.size());
    indexDesc.Usage = D3D11_USAGE_IMMUTABLE;
    indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA indexData{};
    indexData.pSysMem = indices.data();

    if (FAILED(device->CreateBuffer(&indexDesc, &indexData, &mIndexBuffer))) return false;

    mIndexCount = static_cast<uint32_t>(indices.size());

    if (!edges.empty())
    {
      D3D11_BUFFER_DESC edgeDesc = indexDesc;
      edgeDesc.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * edges.size());

      D3D11_SUBRESOURCE_DATA edgeData{};
      edgeData.pSysMem = edges.data();

      if (SUCCEEDED(device->CreateBuffer(&edgeDesc, &edgeData, &mEdgeIndexBuffer)))
      {
        mEdgeIndexCount = static_cast<uint32_t>(edges.size());
      }
    }

    // One small tetrahedron per landmark, rebuilt whenever the markers are shown
    constexpr uint32_t sMarkerVertices = 4U;
    constexpr uint32_t sMarkerIndices = 12U;
    const uint32_t markerCount = static_cast<uint32_t>(mMesh.GetLandmarkPositions().size());

    mMarkerVertices.resize(static_cast<std::size_t>(markerCount) * sMarkerVertices);

    D3D11_BUFFER_DESC markerVertexDesc{};
    markerVertexDesc.ByteWidth = static_cast<UINT>(sizeof(ShaderVertex) * mMarkerVertices.size());
    markerVertexDesc.Usage = D3D11_USAGE_DYNAMIC;
    markerVertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    markerVertexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    if (FAILED(device->CreateBuffer(&markerVertexDesc, nullptr, &mMarkerVertexBuffer))) return false;

    std::vector<uint32_t> markerIndices;
    markerIndices.reserve(static_cast<std::size_t>(markerCount) * sMarkerIndices);

    for (uint32_t m = 0U; m < markerCount; ++m)
    {
      const uint32_t base = m * sMarkerVertices;

      markerIndices.insert(markerIndices.end(), {
        base + 0U, base + 1U, base + 2U,
        base + 0U, base + 2U, base + 3U,
        base + 0U, base + 3U, base + 1U,
        base + 1U, base + 3U, base + 2U });
    }

    D3D11_BUFFER_DESC markerIndexDesc{};
    markerIndexDesc.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * markerIndices.size());
    markerIndexDesc.Usage = D3D11_USAGE_IMMUTABLE;
    markerIndexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA markerIndexData{};
    markerIndexData.pSysMem = markerIndices.data();

    if (FAILED(device->CreateBuffer(&markerIndexDesc, &markerIndexData, &mMarkerIndexBuffer))) return false;

    mMarkerIndexCount = static_cast<uint32_t>(markerIndices.size());

    return true;
  }

  bool HeadView::CreateTargets()
  {
    ID3D11Device1* device = mDevice.GetD3DDevice();
    if (!device || !mSwapChain) return false;

    // A head on a transparent panel is nearly all silhouette, so the edges are what the
    // view is judged by. The highest sample count the adapter offers up to four is taken.
    mSampleCount = 1U;

    for (UINT samples = 4U; samples >= 2U; samples /= 2U)
    {
      UINT quality = 0U;

      if (SUCCEEDED(device->CheckMultisampleQualityLevels(DXGI_FORMAT_B8G8R8A8_UNORM, samples, &quality)) &&
          quality > 0U)
      {
        mSampleCount = samples;
        break;
      }
    }

    D3D11_TEXTURE2D_DESC colorDesc{};
    colorDesc.Width = mWidth;
    colorDesc.Height = mHeight;
    colorDesc.MipLevels = 1U;
    colorDesc.ArraySize = 1U;
    colorDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    colorDesc.SampleDesc.Count = mSampleCount;
    colorDesc.Usage = D3D11_USAGE_DEFAULT;
    colorDesc.BindFlags = D3D11_BIND_RENDER_TARGET;

    if (FAILED(device->CreateTexture2D(&colorDesc, nullptr, &mColorBuffer))) return false;
    if (FAILED(device->CreateRenderTargetView(mColorBuffer.Get(), nullptr, &mColorView))) return false;

    D3D11_TEXTURE2D_DESC depthDesc = colorDesc;
    depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    if (FAILED(device->CreateTexture2D(&depthDesc, nullptr, &mDepthBuffer))) return false;

    return SUCCEEDED(device->CreateDepthStencilView(mDepthBuffer.Get(), nullptr, &mDepthView));
  }

  void HeadView::ReleaseTargets()
  {
    mDepthView.Reset();
    mDepthBuffer.Reset();
    mColorView.Reset();
    mColorBuffer.Reset();
  }

  bool HeadView::Resize(uint32_t iWidth, uint32_t iHeight, double iScaleX, double iScaleY)
  {
    if (!mSwapChain) return true;

    const uint32_t width = (std::max)(1U, static_cast<uint32_t>(std::lround(iWidth * (std::max)(0.1, iScaleX))));
    const uint32_t height = (std::max)(1U, static_cast<uint32_t>(std::lround(iHeight * (std::max)(0.1, iScaleY))));

    if (width == mWidth && height == mHeight) return true;

    ReleaseTargets();

    const HRESULT hr = mSwapChain->ResizeBuffers(0U, width, height, DXGI_FORMAT_UNKNOWN, 0U);
    if (FAILED(hr)) return !GraphicsDevice::IsDeviceLost(hr);

    mWidth = width;
    mHeight = height;

    return CreateTargets();
  }

  void HeadView::SetOptions(const FeHeadOptions& iOptions)
  {
    mOptions = iOptions;
  }

  bool HeadView::UploadMesh()
  {
    ID3D11DeviceContext1* context = mDevice.GetD3DContext();
    if (!context || !mVertexBuffer) return false;

    const auto& vertices = mMesh.GetVertices();
    if (vertices.size() != mShaderVertices.size()) return false;

    for (std::size_t i = 0U; i < vertices.size(); ++i)
    {
      ShaderVertex& target = mShaderVertices[i];

      target.position[0] = vertices[i].position[0];
      target.position[1] = vertices[i].position[1];
      target.position[2] = vertices[i].position[2];

      target.normal[0] = vertices[i].normal[0];
      target.normal[1] = vertices[i].normal[1];
      target.normal[2] = vertices[i].normal[2];

      target.material = vertices[i].material;
      target.occlusion = vertices[i].occlusion;
    }

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(context->Map(mVertexBuffer.Get(), 0U, D3D11_MAP_WRITE_DISCARD, 0U, &mapped))) return false;

    std::memcpy(mapped.pData, mShaderVertices.data(), sizeof(ShaderVertex) * mShaderVertices.size());
    context->Unmap(mVertexBuffer.Get(), 0U);

    return true;
  }

  void HeadView::BuildLandmarkMarkers()
  {
    const auto& landmarks = mMesh.GetLandmarkPositions();

    // A tetrahedron rather than a sphere: four vertices read as a solid point at this size
    // and cost nothing to rebuild every frame
    constexpr float sSize = 0.022F;

    const float corners[4][3] = {
      { 0.0F, -sSize * 1.4F, 0.0F },
      { -sSize, sSize * 0.6F, -sSize * 0.6F },
      { sSize, sSize * 0.6F, -sSize * 0.6F },
      { 0.0F, sSize * 0.6F, sSize * 1.1F }
    };

    for (std::size_t l = 0U; l < landmarks.size() && (l + 1U) * 4U <= mMarkerVertices.size(); ++l)
    {
      for (int c = 0; c < 4; ++c)
      {
        ShaderVertex& vertex = mMarkerVertices[l * 4U + static_cast<std::size_t>(c)];

        vertex.position[0] = landmarks[l][0] + corners[c][0];
        vertex.position[1] = landmarks[l][1] + corners[c][1];
        vertex.position[2] = landmarks[l][2] + corners[c][2] - 0.01F;

        vertex.normal[0] = corners[c][0];
        vertex.normal[1] = corners[c][1];
        vertex.normal[2] = corners[c][2];

        // Material 5 is the marker colour, which is the accent of the user being followed
        vertex.material = 5.0F;
        vertex.occlusion = 1.0F;
      }
    }

    ID3D11DeviceContext1* context = mDevice.GetD3DContext();
    if (!context || !mMarkerVertexBuffer) return;

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(context->Map(mMarkerVertexBuffer.Get(), 0U, D3D11_MAP_WRITE_DISCARD, 0U, &mapped))) return;

    std::memcpy(mapped.pData, mMarkerVertices.data(), sizeof(ShaderVertex) * mMarkerVertices.size());
    context->Unmap(mMarkerVertexBuffer.Get(), 0U);
  }

  void HeadView::UpdateConstants(const face::FaceResult* iFace, double iElapsedSeconds, bool iWireframePass)
  {
    ID3D11DeviceContext1* context = mDevice.GetD3DContext();
    if (!context || !mConstantBuffer) return;

    const XMVECTOR centre = XMVectorSet(mMesh.GetCentre()[0], mMesh.GetCentre()[1], mMesh.GetCentre()[2], 0.0F);

    // The pose rotates the model into the camera, and scaling the rotation vector before it
    // is turned into a matrix is exactly a partial rotation - which is what "follow the pose
    // this much" means
    XMMATRIX pose = XMMatrixIdentity();

    if (iFace && !iFace->rvec.empty() && iFace->rvec.total() >= 3U)
    {
      cv::Mat rvec;
      iFace->rvec.convertTo(rvec, CV_64F);

      const double follow = (std::max)(0.0, (std::min)(1.0, mOptions.poseFollow));

      const double* r = rvec.ptr<double>(0);
      const XMVECTOR axisAngle = XMVectorSet(static_cast<float>(r[0] * follow),
                                             static_cast<float>(r[1] * follow),
                                             static_cast<float>(r[2] * follow), 0.0F);

      const float angle = XMVectorGetX(XMVector3Length(axisAngle));

      if (angle > 1e-5F)
      {
        pose = XMMatrixRotationAxis(XMVector3Normalize(axisAngle), angle);
      }
    }
    else if (mOptions.idleSpin != 0)
    {
      mIdleAngle += iElapsedSeconds * 0.35;
      pose = XMMatrixRotationY(static_cast<float>(std::sin(mIdleAngle) * 0.55));
    }

    const XMMATRIX orbit = XMMatrixRotationX(static_cast<float>(mOptions.orbitPitchDeg) * sDegToRad) *
                           XMMatrixRotationY(static_cast<float>(mOptions.orbitYawDeg) * sDegToRad);

    // Turned about the head's own centre rather than about the nose tip the model is
    // measured from, so that orbiting reads as circling the head
    const XMMATRIX world = XMMatrixTranslationFromVector(XMVectorNegate(centre)) * pose * orbit *
                           XMMatrixTranslationFromVector(centre);

    const float distance = static_cast<float>((std::max)(1.6, (std::min)(8.0, mOptions.distance))) * mMesh.GetRadius();

    // y grows downwards in this space, so the camera's up is -y. With the eye in front of
    // the face and the look direction along +z this is a left-handed frame, and it puts the
    // user's left where a mirror would - which is where the video view puts it too.
    const XMVECTOR eye = XMVectorAdd(centre, XMVectorSet(0.0F, 0.0F, -distance, 0.0F));
    const XMMATRIX view = XMMatrixLookAtLH(eye, centre, XMVectorSet(0.0F, -1.0F, 0.0F, 0.0F));

    const float aspect = static_cast<float>(mWidth) / static_cast<float>((std::max)(1U, mHeight));
    const XMMATRIX projection = XMMatrixPerspectiveFovLH(38.0F * sDegToRad, aspect, 0.05F, 100.0F);

    Constants constants{};

    StoreMatrix(world * view * projection, constants.worldViewProjection);
    StoreMatrix(world, constants.world);

    XMFLOAT3 eyePosition;
    XMStoreFloat3(&eyePosition, eye);

    constants.cameraPosition[0] = eyePosition.x;
    constants.cameraPosition[1] = eyePosition.y;
    constants.cameraPosition[2] = eyePosition.z;
    constants.cameraPosition[3] = 1.0F;

    // From the upper front left, the direction a face is usually lit from
    constants.lightDirection[0] = 0.38F;
    constants.lightDirection[1] = 0.62F;
    constants.lightDirection[2] = 0.68F;
    constants.lightDirection[3] = 0.0F;

    ComputeAccent(iFace ? iFace->userId : 0, constants.accent);

    // The contour, and the colour the shading is allowed to fall to. A cartoon outline is
    // a dark version of the skin rather than black, which would read as a hole.
    constants.outline[0] = 0.16F;
    constants.outline[1] = 0.09F;
    constants.outline[2] = 0.11F;
    constants.outline[3] = 1.0F;

    constants.params[0] = iWireframePass ? 1.0F : 0.0F;
    constants.params[1] = static_cast<float>(mIdleAngle);

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(context->Map(mConstantBuffer.Get(), 0U, D3D11_MAP_WRITE_DISCARD, 0U, &mapped))) return;

    std::memcpy(mapped.pData, &constants, sizeof(constants));
    context->Unmap(mConstantBuffer.Get(), 0U);
  }

  bool HeadView::Render(const ViewFramePtr& iFrame, double iElapsedSeconds)
  {
    ID3D11DeviceContext1* context = mDevice.GetD3DContext();

    if (!context || !mSwapChain || !mColorView || !mDepthView) return true;

    const face::FaceResult* face = nullptr;

    if (iFrame)
    {
      for (const auto& candidate : iFrame->faces)
      {
        // The head follows the face with a usable pose to be oriented by and a normalised
        // shape to be deformed by; among several, the first is the one the pipeline ranked
        // highest
        if (candidate.HasPose() && !candidate.normShape3D.empty())
        {
          face = &candidate;
          break;
        }
      }
    }

    face::HeadMesh::DriveInput drive;
    drive.normalisedShape = face ? &face->normShape3D : nullptr;
    drive.expression = mOptions.expression;

    // This view is presented at the display's rate, which is several times what the pipeline
    // produces, so most calls carry a shape the mesh has already seen. Telling it so is what
    // keeps the smoothing a property of the pipeline rather than of the frame rate.
    const uint32_t frameId = iFrame ? iFrame->frameId : 0U;

    drive.shapeIsNew = (frameId != mLastDrivenFrameId);
    mLastDrivenFrameId = frameId;

    mMesh.Drive(drive);

    if (!UploadMesh()) return true;

    const float clear[4] = { 0.0F, 0.0F, 0.0F, 0.0F };

    context->ClearRenderTargetView(mColorView.Get(), clear);
    context->ClearDepthStencilView(mDepthView.Get(), D3D11_CLEAR_DEPTH, 1.0F, 0U);

    ID3D11RenderTargetView* targets[] = { mColorView.Get() };
    context->OMSetRenderTargets(1U, targets, mDepthView.Get());

    const D3D11_VIEWPORT viewport = { 0.0F, 0.0F, static_cast<float>(mWidth), static_cast<float>(mHeight), 0.0F, 1.0F };
    context->RSSetViewports(1U, &viewport);
    context->RSSetState(mRasterizer.Get());

    context->OMSetDepthStencilState(mDepthState.Get(), 0U);
    context->OMSetBlendState(mBlendState.Get(), nullptr, 0xFFFFFFFFU);

    context->IASetInputLayout(mInputLayout.Get());
    context->VSSetShader(mVertexShader.Get(), nullptr, 0U);
    context->PSSetShader(mPixelShader.Get(), nullptr, 0U);

    ID3D11Buffer* constants[] = { mConstantBuffer.Get() };
    context->VSSetConstantBuffers(0U, 1U, constants);
    context->PSSetConstantBuffers(0U, 1U, constants);

    const UINT stride = sizeof(ShaderVertex);
    const UINT offset = 0U;

    UpdateConstants(face, iElapsedSeconds, false);

    ID3D11Buffer* vertexBuffers[] = { mVertexBuffer.Get() };
    context->IASetVertexBuffers(0U, 1U, vertexBuffers, &stride, &offset);
    context->IASetIndexBuffer(mIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0U);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->DrawIndexed(mMesh.GetSurfaceIndexCount(), 0U, 0);

    if (mOptions.showEyes != 0 && mMesh.GetEyeIndexCount() > 0U)
    {
      context->DrawIndexed(mMesh.GetEyeIndexCount(), mMesh.GetEyeIndexOffset(), 0);
    }

    if (mOptions.showWireframe != 0 && mEdgeIndexCount > 0U)
    {
      UpdateConstants(face, 0.0, true);

      context->IASetIndexBuffer(mEdgeIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0U);
      context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
      context->DrawIndexed(mEdgeIndexCount, 0U, 0);
    }

    if (mOptions.showLandmarks != 0 && mMarkerIndexCount > 0U)
    {
      BuildLandmarkMarkers();
      UpdateConstants(face, 0.0, false);

      ID3D11Buffer* markerBuffers[] = { mMarkerVertexBuffer.Get() };
      context->IASetVertexBuffers(0U, 1U, markerBuffers, &stride, &offset);
      context->IASetIndexBuffer(mMarkerIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0U);
      context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      context->DrawIndexed(mMarkerIndexCount, 0U, 0);
    }

    ComPtr<ID3D11Texture2D> backBuffer;
    if (FAILED(mSwapChain->GetBuffer(0U, IID_PPV_ARGS(&backBuffer)))) return true;

    if (mSampleCount > 1U)
    {
      context->ResolveSubresource(backBuffer.Get(), 0U, mColorBuffer.Get(), 0U, DXGI_FORMAT_B8G8R8A8_UNORM);
    }
    else
    {
      context->CopyResource(backBuffer.Get(), mColorBuffer.Get());
    }

    // Nothing else draws into this chain, and leaving the target bound would keep the back
    // buffer referenced across the present
    ID3D11RenderTargetView* none[] = { nullptr };
    context->OMSetRenderTargets(1U, none, nullptr);

    const HRESULT presentHr = mSwapChain->Present(0U, 0U);
    return !GraphicsDevice::IsDeviceLost(presentHr);
  }

  void HeadView::ComputeAccent(int iUserId, float oAccent[4])
  {
    // The same five colours the video view uses, so a face and its head are one thing
    static const float sAccents[5][3] = {
      { 0.30F, 0.78F, 1.00F },
      { 0.55F, 0.94F, 0.60F },
      { 1.00F, 0.73F, 0.35F },
      { 0.94F, 0.55F, 0.80F },
      { 1.00F, 0.47F, 0.47F }
    };

    const std::size_t index = static_cast<std::size_t>(iUserId < 0 ? -iUserId : iUserId) % 5U;

    oAccent[0] = sAccents[index][0];
    oAccent[1] = sAccents[index][1];
    oAccent[2] = sAccents[index][2];
    oAccent[3] = 1.0F;
  }
}
