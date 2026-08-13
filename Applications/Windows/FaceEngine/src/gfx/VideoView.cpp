#include "gfx/VideoView.h"

#include "Model/FaceModel.h"
#include "Model/PoseGeometry.h"
#include "Model/ShapeMetrics.h"

#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>
#include <string>

namespace fe::gfx
{
  namespace
  {
    /// @brief Picked to stay apart from one another and from skin tones
    constexpr D2D1_COLOR_F sAccents[] = {
      { 0.30F, 0.78F, 1.00F, 1.0F }, // sky
      { 0.55F, 0.94F, 0.60F, 1.0F }, // mint
      { 1.00F, 0.73F, 0.35F, 1.0F }, // amber
      { 0.94F, 0.55F, 0.80F, 1.0F }, // orchid
      { 1.00F, 0.47F, 0.47F, 1.0F }  // coral
    };

    /// @brief The colours of the model's x, y and z axes, in that order
    constexpr D2D1_COLOR_F sAxisColors[] = {
      { 1.00F, 0.42F, 0.42F, 1.0F },
      { 0.45F, 0.90F, 0.50F, 1.0F },
      { 0.40F, 0.70F, 1.00F, 1.0F }
    };

    inline D2D1_COLOR_F WithAlpha(const D2D1_COLOR_F& iColor, float iAlpha)
    {
      return { iColor.r, iColor.g, iColor.b, iAlpha };
    }

    inline D2D1_POINT_2F Transform(const D2D1_MATRIX_3X2_F& iMatrix, double iX, double iY)
    {
      const float x = static_cast<float>(iX);
      const float y = static_cast<float>(iY);

      return {
        x * iMatrix.m11 + y * iMatrix.m21 + iMatrix.dx,
        x * iMatrix.m12 + y * iMatrix.m22 + iMatrix.dy
      };
    }
  }

  VideoView::VideoView(GraphicsDevice& ioDevice) :
    mDevice(ioDevice)
  {
    mOptions.glowStrength = 1.0;
    mOptions.pointSize = 1.6;
    mOptions.showMesh = 1;
    mOptions.showPoints = 1;
    mOptions.showRect = 1;
    mOptions.showPoseBox = 1;
    mOptions.showAxes = 1;
    mOptions.showLabels = 1;
    mOptions.mirror = 1;
  }

  VideoView::~VideoView()
  {
    Destroy();
  }

  bool VideoView::Create(uint32_t iWidth, uint32_t iHeight)
  {
    Destroy();

    mWidth = (std::max)(1U, iWidth);
    mHeight = (std::max)(1U, iHeight);

    if (!mDevice.CreateCompositionSwapChain(mWidth, mHeight, &mSwapChain)) return false;

    if (FAILED(mDevice.GetD2DDevice()->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &mContext)))
    {
      Destroy();
      return false;
    }

    if (!CreateTarget())
    {
      Destroy();
      return false;
    }

    if (FAILED(mContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &mBrush)))
    {
      Destroy();
      return false;
    }

    // Round joins and caps are what make a stroked polyline read as a drawn line rather
    // than as a chain of segments
    D2D1_STROKE_STYLE_PROPERTIES strokeProps = D2D1::StrokeStyleProperties();
    strokeProps.startCap = D2D1_CAP_STYLE_ROUND;
    strokeProps.endCap = D2D1_CAP_STYLE_ROUND;
    strokeProps.lineJoin = D2D1_LINE_JOIN_ROUND;

    ComPtr<ID2D1Factory> factory;
    mContext->GetFactory(&factory);

    if (!factory || FAILED(factory->CreateStrokeStyle(strokeProps, nullptr, 0U, &mRoundStroke)))
    {
      Destroy();
      return false;
    }

    IDWriteFactory1* write = mDevice.GetWriteFactory();

    // Not every Windows build carries the variable family, so the classic one is the fallback
    if (FAILED(write->CreateTextFormat(L"Segoe UI Variable Text", nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD,
                                       DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                       11.0F, L"en-us", &mLabelFormat)) &&
        FAILED(write->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD,
                                       DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                       11.0F, L"en-us", &mLabelFormat)))
    {
      Destroy();
      return false;
    }

    mLabelFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    mLabelFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    if (SUCCEEDED(write->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
                                          DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                          14.0F, L"en-us", &mEmptyFormat)))
    {
      mEmptyFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
      mEmptyFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    return true;
  }

  void VideoView::Destroy()
  {
    ReleaseTarget();

    mEmptyFormat.Reset();
    mLabelFormat.Reset();
    mRoundStroke.Reset();
    mBrush.Reset();
    mSource.Reset();
    mContext.Reset();
    mSwapChain.Reset();

    mSourceWidth = 0U;
    mSourceHeight = 0U;
  }

  bool VideoView::CreateTarget()
  {
    if (!mSwapChain || !mContext) return false;

    ComPtr<IDXGISurface> surface;
    if (FAILED(mSwapChain->GetBuffer(0U, IID_PPV_ARGS(&surface)))) return false;

    const D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
      D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
      D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

    if (FAILED(mContext->CreateBitmapFromDxgiSurface(surface.Get(), &props, &mTarget))) return false;

    mContext->SetTarget(mTarget.Get());
    return true;
  }

  void VideoView::ReleaseTarget()
  {
    if (mContext) mContext->SetTarget(nullptr);
    mTarget.Reset();
  }

  bool VideoView::Resize(uint32_t iWidth, uint32_t iHeight, double iScaleX, double iScaleY)
  {
    if (!mSwapChain) return true;

    const uint32_t width = (std::max)(1U, static_cast<uint32_t>(std::lround(iWidth * (std::max)(0.1, iScaleX))));
    const uint32_t height = (std::max)(1U, static_cast<uint32_t>(std::lround(iHeight * (std::max)(0.1, iScaleY))));

    if (width == mWidth && height == mHeight) return true;

    // The context must let go of the back buffer before DXGI can replace it
    ReleaseTarget();

    const HRESULT hr = mSwapChain->ResizeBuffers(0U, width, height, DXGI_FORMAT_UNKNOWN, 0U);
    if (FAILED(hr)) return !GraphicsDevice::IsDeviceLost(hr);

    mWidth = width;
    mHeight = height;

    return CreateTarget();
  }

  void VideoView::SetOptions(const FeOverlayOptions& iOptions)
  {
    mOptions = iOptions;
  }

  bool VideoView::UpdateSourceBitmap(const cv::Mat& iImage)
  {
    if (iImage.empty() || !mContext) return false;

    const uint32_t width = static_cast<uint32_t>(iImage.cols);
    const uint32_t height = static_cast<uint32_t>(iImage.rows);

    // Direct2D wants a straight BGRA block. The conversion writes into a buffer that is
    // reused, so a steady stream of frames of one size allocates nothing.
    if (iImage.type() == CV_8UC3)
    {
      cv::cvtColor(iImage, mUploadBuffer, cv::COLOR_BGR2BGRA);
    }
    else if (iImage.type() == CV_8UC1)
    {
      cv::cvtColor(iImage, mUploadBuffer, cv::COLOR_GRAY2BGRA);
    }
    else if (iImage.type() == CV_8UC4)
    {
      mUploadBuffer = iImage;
    }
    else
    {
      return false;
    }

    if (!mSource || mSourceWidth != width || mSourceHeight != height)
    {
      mSource.Reset();

      const D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_NONE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE));

      if (FAILED(mContext->CreateBitmap(D2D1::SizeU(width, height), nullptr, 0U, props, &mSource))) return false;

      mSourceWidth = width;
      mSourceHeight = height;
    }

    const D2D1_RECT_U full = D2D1::RectU(0U, 0U, width, height);

    return SUCCEEDED(mSource->CopyFromMemory(&full, mUploadBuffer.data,
                                             static_cast<UINT32>(mUploadBuffer.step)));
  }

  D2D1_RECT_F VideoView::ComputeFitRect() const
  {
    const float viewW = static_cast<float>(mWidth);
    const float viewH = static_cast<float>(mHeight);

    if (mSourceWidth == 0U || mSourceHeight == 0U) return D2D1::RectF(0.0F, 0.0F, viewW, viewH);

    const float srcW = static_cast<float>(mSourceWidth);
    const float srcH = static_cast<float>(mSourceHeight);

    const float scale = (std::min)(viewW / srcW, viewH / srcH);
    const float drawW = srcW * scale;
    const float drawH = srcH * scale;

    const float left = (viewW - drawW) * 0.5F;
    const float top = (viewH - drawH) * 0.5F;

    return D2D1::RectF(left, top, left + drawW, top + drawH);
  }

  bool VideoView::Render(const ViewFramePtr& iFrame)
  {
    if (!mContext || !mTarget || !mSwapChain) return true;

    const bool hasFrame = iFrame && !iFrame->image.empty() && UpdateSourceBitmap(iFrame->image);

    mContext->BeginDraw();
    mContext->SetTransform(D2D1::Matrix3x2F::Identity());

    // Transparent rather than a colour: the panel behind it is the page background, so the
    // letterbox bars are the application's own surface instead of a black frame
    mContext->Clear(D2D1::ColorF(0.0F, 0.0F, 0.0F, 0.0F));

    if (hasFrame)
    {
      const D2D1_RECT_F fit = ComputeFitRect();
      const bool mirrored = mOptions.mirror != 0;

      {
        std::lock_guard<std::mutex> lock(mMapMutex);
        mLastFitRect = fit;
        mLastMirrored = mirrored;
      }

      // Frame pixels to view pixels, with the mirroring folded in so that the landmarks
      // travel with the image instead of having to be flipped separately
      const float scale = (fit.right - fit.left) / static_cast<float>(mSourceWidth);

      D2D1::Matrix3x2F toView = D2D1::Matrix3x2F::Scale(scale, scale) *
                                D2D1::Matrix3x2F::Translation(fit.left, fit.top);

      if (mirrored)
      {
        toView = D2D1::Matrix3x2F::Scale(-1.0F, 1.0F) *
                 D2D1::Matrix3x2F::Translation(static_cast<float>(mSourceWidth), 0.0F) *
                 toView;
      }

      // Only the bitmap is drawn under a mirroring transform; the overlay maps its own
      // points, which is what keeps the labels upright and readable
      if (mirrored)
      {
        const D2D1::Matrix3x2F flip = D2D1::Matrix3x2F::Scale(-1.0F, 1.0F) *
                                      D2D1::Matrix3x2F::Translation(fit.left + fit.right, 0.0F);
        mContext->SetTransform(flip);
      }

      mContext->DrawBitmap(mSource.Get(), fit, 1.0F, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
      mContext->SetTransform(D2D1::Matrix3x2F::Identity());

      // The pipeline already burned its own overlay into these pixels
      if (!iFrame->imageHasOverlay)
      {
        for (const auto& face : iFrame->faces)
        {
          DrawFace(face, toView, AccentOf(face.userId));
        }
      }
    }
    else
    {
      DrawEmptyState();
    }

    const HRESULT endHr = mContext->EndDraw();
    if (FAILED(endHr)) return !GraphicsDevice::IsDeviceLost(endHr);

    // Before the present, not after: a flip-model back buffer is undefined once presented
    CaptureBackBuffer();

    // Without waiting for the vertical blank: the engine's render thread holds a lock
    // across this call and paces itself with a sleep, so a blocking present here would
    // keep every other thread waiting on the display's refresh
    const HRESULT presentHr = mSwapChain->Present(0U, 0U);
    return !GraphicsDevice::IsDeviceLost(presentHr);
  }

  void VideoView::SetCaptureEnabled(bool iEnabled)
  {
    std::lock_guard<std::mutex> lock(mCaptureMutex);
    mCaptureEnabled = iEnabled;
  }

  void VideoView::RequestCapture()
  {
    std::lock_guard<std::mutex> lock(mCaptureMutex);
    mCaptureRequested = true;
  }

  bool VideoView::TakeCapture(cv::Mat& oImage)
  {
    std::lock_guard<std::mutex> lock(mCaptureMutex);

    if (mCapturedFrame.empty()) return false;

    oImage = mCapturedFrame;
    mCapturedFrame.release();

    return true;
  }

  void VideoView::CaptureBackBuffer()
  {
    {
      std::lock_guard<std::mutex> lock(mCaptureMutex);
      if (!mCaptureEnabled && !mCaptureRequested) return;
    }

    ID3D11Device1* device = mDevice.GetD3DDevice();
    ID3D11DeviceContext1* context = mDevice.GetD3DContext();

    if (!device || !context || !mSwapChain) return;

    ComPtr<ID3D11Texture2D> backBuffer;
    if (FAILED(mSwapChain->GetBuffer(0U, IID_PPV_ARGS(&backBuffer)))) return;

    if (!mStagingTexture || mStagingWidth != mWidth || mStagingHeight != mHeight)
    {
      mStagingTexture.Reset();

      D3D11_TEXTURE2D_DESC desc{};
      backBuffer->GetDesc(&desc);

      desc.Usage = D3D11_USAGE_STAGING;
      desc.BindFlags = 0U;
      desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
      desc.MiscFlags = 0U;

      if (FAILED(device->CreateTexture2D(&desc, nullptr, &mStagingTexture))) return;

      mStagingWidth = mWidth;
      mStagingHeight = mHeight;
    }

    context->CopyResource(mStagingTexture.Get(), backBuffer.Get());

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(context->Map(mStagingTexture.Get(), 0U, D3D11_MAP_READ, 0U, &mapped))) return;

    // BGRA on the GPU, BGR for the writers, and the alpha of a composited view is not
    // something a video file can carry anyway
    const cv::Mat view(static_cast<int>(mHeight), static_cast<int>(mWidth), CV_8UC4,
                       mapped.pData, mapped.RowPitch);

    cv::Mat converted;
    cv::cvtColor(view, converted, cv::COLOR_BGRA2BGR);

    context->Unmap(mStagingTexture.Get(), 0U);

    std::lock_guard<std::mutex> lock(mCaptureMutex);
    mCapturedFrame = std::move(converted);
    mCaptureRequested = false;
  }

  void VideoView::DrawFace(const face::FaceResult& iFace, const D2D1_MATRIX_3X2_F& iToView, const D2D1_COLOR_F& iAccent)
  {
    if (mOptions.showPoseBox) DrawPoseBox(iFace, iToView, iAccent);
    if (mOptions.showRect) DrawCornerBrackets(iFace, iToView, iAccent);
    if (mOptions.showMesh || mOptions.showPoints) DrawFeatureStrokes(iFace, iToView, iAccent);
    if (mOptions.showAxes) DrawAxes(iFace, iToView);
    if (mOptions.showLabels) DrawLabel(iFace, iToView, iAccent);
  }

  void VideoView::StrokeGlowing(ID2D1Geometry* iGeometry, const D2D1_COLOR_F& iColor, float iWidth)
  {
    if (!iGeometry) return;

    const float glow = static_cast<float>((std::max)(0.0, (std::min)(2.0, mOptions.glowStrength)));

    if (glow > 0.01F)
    {
      mBrush->SetColor(WithAlpha(iColor, 0.16F * glow));
      mContext->DrawGeometry(iGeometry, mBrush.Get(), iWidth + 5.0F * glow, mRoundStroke.Get());

      mBrush->SetColor(WithAlpha(iColor, 0.26F * glow));
      mContext->DrawGeometry(iGeometry, mBrush.Get(), iWidth + 2.0F * glow, mRoundStroke.Get());
    }

    mBrush->SetColor(iColor);
    mContext->DrawGeometry(iGeometry, mBrush.Get(), iWidth, mRoundStroke.Get());
  }

  void VideoView::DrawFeatureStrokes(const face::FaceResult& iFace, const D2D1_MATRIX_3X2_F& iToView, const D2D1_COLOR_F& iAccent)
  {
    const auto& shape = iFace.shape2D;
    if (shape.empty()) return;

    const int count = static_cast<int>(shape.size());

    ComPtr<ID2D1Factory> factory;
    mContext->GetFactory(&factory);
    if (!factory) return;

    if (mOptions.showMesh)
    {
      // Which points join into which stroke is a property of the layout, so it comes from
      // the model rather than being described again here
      for (const auto& stroke : face::FaceModel::GetInstance().GetFeatureStrokes())
      {
        if (stroke.last >= count) continue;

        ComPtr<ID2D1PathGeometry> path;
        if (FAILED(factory->CreatePathGeometry(&path))) continue;

        ComPtr<ID2D1GeometrySink> sink;
        if (FAILED(path->Open(&sink))) continue;

        sink->BeginFigure(Transform(iToView, shape[stroke.first].x, shape[stroke.first].y),
                          D2D1_FIGURE_BEGIN_HOLLOW);

        for (int i = stroke.first + 1; i <= stroke.last; ++i)
        {
          sink->AddLine(Transform(iToView, shape[i].x, shape[i].y));
        }

        sink->EndFigure(stroke.closed ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN);
        sink->Close();

        StrokeGlowing(path.Get(), iAccent, 1.6F);
      }
    }

    if (!mOptions.showPoints) return;

    const float radius = static_cast<float>((std::max)(0.5, (std::min)(6.0, mOptions.pointSize)));

    for (int i = 0; i < count; ++i)
    {
      const D2D1_POINT_2F pt = Transform(iToView, shape[i].x, shape[i].y);

      // A dark disc under a light one, so a point stays visible on a bright face too
      mBrush->SetColor(D2D1::ColorF(0.0F, 0.0F, 0.0F, 0.55F));
      mContext->FillEllipse(D2D1::Ellipse(pt, radius + 0.9F, radius + 0.9F), mBrush.Get());

      mBrush->SetColor(D2D1::ColorF(1.0F, 1.0F, 1.0F, 0.92F));
      mContext->FillEllipse(D2D1::Ellipse(pt, radius, radius), mBrush.Get());
    }
  }

  void VideoView::DrawCornerBrackets(const face::FaceResult& iFace, const D2D1_MATRIX_3X2_F& iToView, const D2D1_COLOR_F& iAccent)
  {
    if (iFace.faceRect.width <= 0 || iFace.faceRect.height <= 0) return;

    const auto brackets = fw::corner_brackets(cv::Rect2d(iFace.faceRect));

    mBrush->SetColor(WithAlpha(iAccent, 0.85F));

    for (const auto& bracket : brackets)
    {
      const D2D1_POINT_2F a = Transform(iToView, bracket[0].x, bracket[0].y);
      const D2D1_POINT_2F b = Transform(iToView, bracket[1].x, bracket[1].y);
      const D2D1_POINT_2F c = Transform(iToView, bracket[2].x, bracket[2].y);

      mContext->DrawLine(a, b, mBrush.Get(), 2.0F, mRoundStroke.Get());
      mContext->DrawLine(b, c, mBrush.Get(), 2.0F, mRoundStroke.Get());
    }
  }

  void VideoView::DrawPoseBox(const face::FaceResult& iFace, const D2D1_MATRIX_3X2_F& iToView, const D2D1_COLOR_F& iAccent)
  {
    if (!iFace.HasPose() || iFace.faceBox.size() < 8U) return;

    try
    {
      cv::projectPoints(iFace.faceBox, iFace.rvec, iFace.tvec, iFace.cameraMatrix, cv::noArray(), mProjectedBox);
    }
    catch (const cv::Exception&)
    {
      return;
    }

    const int corners = static_cast<int>(mProjectedBox.size());

    mBrush->SetColor(WithAlpha(iAccent, 0.30F));

    for (const auto& edge : face::PoseGeometry::GetInstance().GetConnections())
    {
      if (edge.first < 0 || edge.first >= corners || edge.second < 0 || edge.second >= corners) continue;

      mContext->DrawLine(Transform(iToView, mProjectedBox[edge.first].x, mProjectedBox[edge.first].y),
                         Transform(iToView, mProjectedBox[edge.second].x, mProjectedBox[edge.second].y),
                         mBrush.Get(), 1.0F, mRoundStroke.Get());
    }
  }

  void VideoView::DrawAxes(const face::FaceResult& iFace, const D2D1_MATRIX_3X2_F& iToView)
  {
    const std::size_t noseTip = static_cast<std::size_t>(face::index_of(face::Landmark::kNose3));

    if (!iFace.HasPose() || iFace.shape2D.size() <= noseTip) return;

    cv::Mat rotation;
    try
    {
      cv::Rodrigues(iFace.rvec, rotation);
    }
    catch (const cv::Exception&)
    {
      return;
    }

    const auto axes = face::project_axes(rotation);
    if (axes.empty()) return;

    const D2D1_POINT_2F origin = Transform(iToView, iFace.shape2D[noseTip].x, iFace.shape2D[noseTip].y);

    // The overlay is drawn in view pixels, so the gizmo is sized against the face as it
    // appears rather than against the frame it came from
    const float scale = std::abs(iToView.m11);
    const float length = iFace.faceRect.width > 0
                           ? (std::max)(18.0F, static_cast<float>(iFace.faceRect.width) * 0.30F * scale)
                           : 28.0F;

    // A mirrored view flips which way an axis points on screen
    const float signX = iToView.m11 < 0.0F ? -1.0F : 1.0F;

    for (const auto& axis : axes)
    {
      const D2D1_POINT_2F tip = {
        origin.x + static_cast<float>(axis.direction.x) * length * signX,
        origin.y + static_cast<float>(axis.direction.y) * length
      };

      const float fade = axis.away > 0.0 ? 0.45F : 1.0F;

      mBrush->SetColor(D2D1::ColorF(0.0F, 0.0F, 0.0F, 0.45F * fade));
      mContext->DrawLine(origin, tip, mBrush.Get(), 3.5F, mRoundStroke.Get());

      mBrush->SetColor(WithAlpha(sAxisColors[axis.index], fade));
      mContext->DrawLine(origin, tip, mBrush.Get(), 2.0F, mRoundStroke.Get());
      mContext->FillEllipse(D2D1::Ellipse(tip, 2.2F, 2.2F), mBrush.Get());
    }
  }

  void VideoView::DrawLabel(const face::FaceResult& iFace, const D2D1_MATRIX_3X2_F& iToView, const D2D1_COLOR_F& iAccent)
  {
    if (!mLabelFormat || iFace.faceRect.width <= 0) return;

    const D2D1_POINT_2F a = Transform(iToView, iFace.faceRect.x, iFace.faceRect.y);
    const D2D1_POINT_2F b = Transform(iToView, iFace.faceRect.x + iFace.faceRect.width, iFace.faceRect.y);

    const float centreX = (a.x + b.x) * 0.5F;
    const float baseY = (std::min)(a.y, b.y);

    const wchar_t* state = iFace.status == face::TrackStatus::Detected
                             ? L"DETECTED"
                             : (iFace.status == face::TrackStatus::Tracked ? L"TRACKED" : L"INACTIVE");

    const std::wstring text = L"USER " + std::to_wstring(iFace.userId) + L"  \x00b7  " + state;

    const float width = 46.0F + 6.4F * static_cast<float>(text.size());
    const float height = 20.0F;

    const D2D1_RECT_F pill = D2D1::RectF(centreX - width * 0.5F, baseY - height - 8.0F,
                                         centreX + width * 0.5F, baseY - 8.0F);

    mBrush->SetColor(D2D1::ColorF(0.05F, 0.06F, 0.08F, 0.72F));
    mContext->FillRoundedRectangle(D2D1::RoundedRect(pill, height * 0.5F, height * 0.5F), mBrush.Get());

    mBrush->SetColor(WithAlpha(iAccent, 0.55F));
    mContext->DrawRoundedRectangle(D2D1::RoundedRect(pill, height * 0.5F, height * 0.5F), mBrush.Get(), 1.0F);

    // The chip ties the label to the strokes drawn in the same colour
    mBrush->SetColor(iAccent);
    mContext->FillEllipse(D2D1::Ellipse({ pill.left + 11.0F, (pill.top + pill.bottom) * 0.5F }, 3.0F, 3.0F), mBrush.Get());

    mBrush->SetColor(D2D1::ColorF(1.0F, 1.0F, 1.0F, 0.94F));
    mContext->DrawText(text.c_str(), static_cast<UINT32>(text.size()), mLabelFormat.Get(),
                       D2D1::RectF(pill.left + 14.0F, pill.top, pill.right, pill.bottom),
                       mBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
  }

  void VideoView::DrawEmptyState()
  {
    if (!mEmptyFormat || !mBrush) return;

    const float w = static_cast<float>(mWidth);
    const float h = static_cast<float>(mHeight);

    // A soft frame and one line of text, so that an empty view still looks designed
    const D2D1_RECT_F frame = D2D1::RectF(w * 0.5F - 150.0F, h * 0.5F - 60.0F,
                                          w * 0.5F + 150.0F, h * 0.5F + 60.0F);

    mBrush->SetColor(D2D1::ColorF(1.0F, 1.0F, 1.0F, 0.10F));
    mContext->DrawRoundedRectangle(D2D1::RoundedRect(frame, 14.0F, 14.0F), mBrush.Get(), 1.0F);

    static const wchar_t* sText = L"No source\nChoose a camera or open a video";

    mBrush->SetColor(D2D1::ColorF(1.0F, 1.0F, 1.0F, 0.45F));
    mContext->DrawText(sText, static_cast<UINT32>(wcslen(sText)), mEmptyFormat.Get(), frame, mBrush.Get());
  }

  bool VideoView::ViewToFrame(double iViewX, double iViewY, double& oFrameX, double& oFrameY) const
  {
    std::lock_guard<std::mutex> lock(mMapMutex);

    const float width = mLastFitRect.right - mLastFitRect.left;
    if (width <= 0.0F || mSourceWidth == 0U) return false;

    const double scale = static_cast<double>(mSourceWidth) / width;

    oFrameX = (iViewX - mLastFitRect.left) * scale;
    oFrameY = (iViewY - mLastFitRect.top) * scale;

    if (mLastMirrored) oFrameX = mSourceWidth - oFrameX;

    return true;
  }

  D2D1_COLOR_F VideoView::AccentOf(int iUserId)
  {
    const std::size_t count = sizeof(sAccents) / sizeof(sAccents[0]);
    const std::size_t index = static_cast<std::size_t>(iUserId < 0 ? -iUserId : iUserId) % count;

    return sAccents[index];
  }
}
