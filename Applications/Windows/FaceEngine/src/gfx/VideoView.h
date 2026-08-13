#pragma once

#include "FaceEngine.h"
#include "gfx/GraphicsDevice.h"

#include "ViewFrame.h"

#include <mutex>

namespace fe::gfx
{
  /// @brief The camera view: the frame, scaled to fit, with the overlay drawn on top of it.
  ///
  /// Direct2D rather than a shader pass, because the frame is one bitmap draw and everything
  /// else is strokes and text. The frame never travels back to the CPU and is never
  /// composited into an OpenCV image, which is the whole reason the pipeline's own Visualizer
  /// is left out of the graph for this application.
  ///
  /// What to draw - which landmarks join into which stroke, where the pose axes point - comes
  /// from the API's own model; this class only knows how to draw it with Direct2D.
  class VideoView
  {
  public:
    explicit VideoView(GraphicsDevice& ioDevice);

    VideoView(const VideoView& iOther) = delete;

    ~VideoView();

    VideoView& operator=(const VideoView& iOther) = delete;

    /// @brief Creates the swap chain. The caller passes it to ISwapChainPanelNative.
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

    /// @param iScaleX, iScaleY DPI scale of the panel, so the chain is sized in real pixels
    bool Resize(uint32_t iWidth, uint32_t iHeight, double iScaleX, double iScaleY);

    void SetOptions(const FeOverlayOptions& iOptions);

    /// @brief Draws iFrame and presents. A null frame paints the empty state.
    /// @return false when the device was lost and has to be rebuilt
    bool Render(const ViewFramePtr& iFrame);

    /// @brief Maps a point of the view onto the frame, undoing the fit and the mirroring.
    bool ViewToFrame(double iViewX, double iViewY, double& oFrameX, double& oFrameY) const;

    /// @brief Keeps a copy of every rendered view on the CPU, for recording.
    ///
    /// A flip-model back buffer is undefined once it has been presented, so the copy is
    /// taken between the drawing and the present rather than on demand from outside.
    void SetCaptureEnabled(bool iEnabled);

    /// @brief Asks for the next rendered view to be copied, once.
    void RequestCapture();

    /// @brief Hands over the last copy, leaving nothing behind.
    /// @return false when no copy has been taken since the last call
    bool TakeCapture(cv::Mat& oImage);

  private:
    bool CreateTarget();

    void ReleaseTarget();

    bool UpdateSourceBitmap(const cv::Mat& iImage);

    /// @brief Places the frame inside the view, preserving its aspect ratio
    D2D1_RECT_F ComputeFitRect() const;

    void DrawFace(const face::FaceResult& iFace, const D2D1_MATRIX_3X2_F& iToView, const D2D1_COLOR_F& iAccent);

    void DrawFeatureStrokes(const face::FaceResult& iFace, const D2D1_MATRIX_3X2_F& iToView, const D2D1_COLOR_F& iAccent);

    void DrawCornerBrackets(const face::FaceResult& iFace, const D2D1_MATRIX_3X2_F& iToView, const D2D1_COLOR_F& iAccent);

    void DrawPoseBox(const face::FaceResult& iFace, const D2D1_MATRIX_3X2_F& iToView, const D2D1_COLOR_F& iAccent);

    void DrawAxes(const face::FaceResult& iFace, const D2D1_MATRIX_3X2_F& iToView);

    void DrawLabel(const face::FaceResult& iFace, const D2D1_MATRIX_3X2_F& iToView, const D2D1_COLOR_F& iAccent);

    void DrawEmptyState();

    /// @brief Copies the back buffer into mCapturedFrame through a staging texture
    void CaptureBackBuffer();

    /// @brief Strokes a geometry twice: a wide translucent pass that reads as a halo and a
    /// thin opaque one on top. Cheaper than an offscreen blur and it survives any background.
    void StrokeGlowing(ID2D1Geometry* iGeometry, const D2D1_COLOR_F& iColor, float iWidth);

    static D2D1_COLOR_F AccentOf(int iUserId);

    GraphicsDevice& mDevice;

    ComPtr<IDXGISwapChain1> mSwapChain;
    ComPtr<ID2D1DeviceContext> mContext;
    ComPtr<ID2D1Bitmap1> mTarget;

    /// @brief The frame, uploaded once per frame and drawn once
    ComPtr<ID2D1Bitmap1> mSource;

    ComPtr<ID2D1SolidColorBrush> mBrush;
    ComPtr<ID2D1StrokeStyle> mRoundStroke;
    ComPtr<IDWriteTextFormat> mLabelFormat;
    ComPtr<IDWriteTextFormat> mEmptyFormat;

    /// @brief Reused so that the per-frame BGRA conversion does not allocate
    cv::Mat mUploadBuffer;

    /// @brief Reused by the pose box projection for the same reason
    fw::VectorPt2D mProjectedBox;

    uint32_t mWidth = 1U;
    uint32_t mHeight = 1U;
    uint32_t mSourceWidth = 0U;
    uint32_t mSourceHeight = 0U;

    /// @brief The fit of the last Render, kept for ViewToFrame
    mutable std::mutex mMapMutex;
    D2D1_RECT_F mLastFitRect{};
    bool mLastMirrored = false;

    /// @brief Reused between captures, so recording does not reallocate per frame
    ComPtr<ID3D11Texture2D> mStagingTexture;
    uint32_t mStagingWidth = 0U;
    uint32_t mStagingHeight = 0U;

    std::mutex mCaptureMutex;
    cv::Mat mCapturedFrame;
    bool mCaptureEnabled = false;
    bool mCaptureRequested = false;

    FeOverlayOptions mOptions{};
  };
}
