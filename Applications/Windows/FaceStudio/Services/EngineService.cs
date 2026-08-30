using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO;
using System.Runtime.CompilerServices;

using FaceStudio.Interop;
using FaceStudio.Models;

using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml.Controls;

namespace FaceStudio.Services;

/// <summary>
/// The application's one connection to the native engine.
///
/// Everything the UI knows about what is happening comes from here, and it comes on a timer
/// rather than on a callback: the engine runs at its own pace on its own threads, and a UI
/// that redrew a panel per frame would be doing thirty times the work a person can read.
/// The frames themselves never pass through - they are composited straight from the swap
/// chains this binds to the panels.
/// </summary>
public sealed class EngineService : INotifyPropertyChanged, IDisposable
{
    /// <summary>How often the panels are refreshed. Fast enough to look live, far below the
    /// frame rate, which is the point.</summary>
    private static readonly TimeSpan PollInterval = TimeSpan.FromMilliseconds(50);

    /// <summary>Samples kept for the charts: two minutes at the poll rate.</summary>
    private const int HistoryLength = 240;

    private readonly DispatcherQueue _dispatcher;
    private readonly DispatcherQueueTimer? _timer;

    private IntPtr _handle = IntPtr.Zero;

    private SwapChainPanel? _videoPanel;
    private SwapChainPanel? _headPanel;
    private int _boundGeneration;
    private int _stageLayoutVersion = -1;

    private NativeEngine.OverlayOptions _overlay = NativeEngine.OverlayOptions.CreateDefault();
    private NativeEngine.HeadOptions _head = NativeEngine.HeadOptions.CreateDefault();

    private string _statusMessage = string.Empty;
    private bool _isRecording;

    public EngineService(string iWorkingDirectory)
    {
        WorkingDirectory = iWorkingDirectory;

        _dispatcher = DispatcherQueue.GetForCurrentThread();

        Faces = new ObservableCollection<FaceStatus>();
        Stages = new ObservableCollection<StageTiming>();
        Cameras = new ObservableCollection<string>();

        LatencyHistory = new MetricHistory(HistoryLength);
        PipelineFpsHistory = new MetricHistory(HistoryLength);
        CaptureFpsHistory = new MetricHistory(HistoryLength);

        if (!VerifyContract())
        {
            StatusMessage = "FaceEngine.dll does not match what this build expects. " +
                            "Rebuild the solution so that the engine and the application agree.";
            return;
        }

        NativeEngine.Status status = NativeEngine.FeEngine_Create(iWorkingDirectory, out _handle);

        if (status != NativeEngine.Status.Ok)
        {
            // The handle is still valid when initialisation failed - it is what carries the
            // reason - so it is kept and torn down normally
            StatusMessage = ReadLastError() is { Length: > 0 } message
                ? message
                : $"The engine could not be started ({status}).";
            return;
        }

        IsAvailable = true;

        PushOverlayOptions();
        PushHeadOptions();
        RefreshCameras();

        _timer = _dispatcher.CreateTimer();
        _timer.Interval = PollInterval;
        _timer.IsRepeating = true;
        _timer.Tick += (_, _) => Poll();
        _timer.Start();
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    /// <summary>Raised after every poll, for views that redraw rather than bind.</summary>
    public event EventHandler? Updated;

    public string WorkingDirectory { get; }

    /// <summary>False when the engine could not be started; <see cref="StatusMessage"/> says why.</summary>
    public bool IsAvailable { get; }

    public string StatusMessage
    {
        get => _statusMessage;
        private set => Set(ref _statusMessage, value);
    }

    public ObservableCollection<string> Cameras { get; }

    public ObservableCollection<FaceStatus> Faces { get; }

    public ObservableCollection<StageTiming> Stages { get; }

    public MetricHistory LatencyHistory { get; }

    public MetricHistory PipelineFpsHistory { get; }

    public MetricHistory CaptureFpsHistory { get; }

    /// <summary>The last snapshot, as the engine reported it.</summary>
    public NativeEngine.Snapshot Snapshot { get; private set; }

    public bool IsRunning => Snapshot.IsRunning != 0;

    public bool IsPaused => Snapshot.IsPaused != 0;

    public bool IsRecording
    {
        get => _isRecording;
        private set => Set(ref _isRecording, value);
    }

    /// <summary>
    /// True while the pipeline's own Visualizer is drawing the overlay into the frames, in
    /// which case this application leaves them alone and its overlay switches do nothing.
    /// </summary>
    public bool PipelineDrawsOverlay => Snapshot.PipelineDrawsOverlay != 0;

    public NativeEngine.OverlayOptions Overlay
    {
        get => _overlay;
        set
        {
            _overlay = value;
            PushOverlayOptions();
            OnPropertyChanged();
        }
    }

    public NativeEngine.HeadOptions Head
    {
        get => _head;
        set
        {
            _head = value;
            PushHeadOptions();
            OnPropertyChanged();
        }
    }

    /// <summary>
    /// Checks that the structs this build declares are the ones the engine was compiled
    /// with. A mismatch here reads as plausible nonsense in every panel, so it is worth one
    /// call at startup to turn it into a message instead.
    /// </summary>
    private static bool VerifyContract()
    {
        try
        {
            NativeEngine.FeEngine_GetStructSizes(out int snapshotSize, out int faceSize);

            return snapshotSize == System.Runtime.InteropServices.Marshal.SizeOf<NativeEngine.Snapshot>()
                && faceSize == System.Runtime.InteropServices.Marshal.SizeOf<NativeEngine.Face>();
        }
        catch (DllNotFoundException)
        {
            return false;
        }
        catch (EntryPointNotFoundException)
        {
            return false;
        }
    }

    public void RefreshCameras()
    {
        Cameras.Clear();

        if (_handle == IntPtr.Zero) return;

        int count = NativeEngine.FeEngine_EnumerateCameras(_handle, 8);

        for (int i = 0; i < count; i++)
        {
            int index = i;
            Cameras.Add(NativeEngine.ReadString((buffer, capacity) =>
                NativeEngine.FeEngine_GetCameraName(_handle, index, buffer, capacity)));
        }
    }

    public bool OpenCamera(int iIndex, int iWidth = 0, int iHeight = 0, double iFps = 0.0)
    {
        if (_handle == IntPtr.Zero) return false;

        NativeEngine.Status status = NativeEngine.FeEngine_OpenCamera(_handle, iIndex, iWidth, iHeight, iFps);

        return Report(status, $"Camera {iIndex} could not be opened.");
    }

    public bool OpenFile(string iPath)
    {
        if (_handle == IntPtr.Zero) return false;

        return Report(NativeEngine.FeEngine_OpenFile(_handle, iPath), $"{Path.GetFileName(iPath)} could not be opened.");
    }

    public void CloseSource()
    {
        if (_handle == IntPtr.Zero) return;

        NativeEngine.FeEngine_CloseSource(_handle);
        StatusMessage = string.Empty;
    }

    public void SetPaused(bool iPaused)
    {
        if (_handle != IntPtr.Zero) NativeEngine.FeEngine_SetPaused(_handle, iPaused ? 1 : 0);
    }

    public void SetLooping(bool iLooping)
    {
        if (_handle != IntPtr.Zero) NativeEngine.FeEngine_SetLooping(_handle, iLooping ? 1 : 0);
    }

    public void ClearUsers()
    {
        if (_handle != IntPtr.Zero) NativeEngine.FeEngine_ClearUsers(_handle);
    }

    public void ForceDetection()
    {
        if (_handle != IntPtr.Zero) NativeEngine.FeEngine_ForceDetection(_handle);
    }

    public void SetVerbose(bool iVerbose)
    {
        if (_handle != IntPtr.Zero) NativeEngine.FeEngine_SetVerbose(_handle, iVerbose ? 1 : 0);
    }

    /// <summary>Rebuilds the pipeline from settings.json, keeping the source running.</summary>
    public bool ReloadPipeline()
    {
        if (_handle == IntPtr.Zero) return false;

        bool ok = Report(NativeEngine.FeEngine_ReloadPipeline(_handle), "The pipeline could not be rebuilt.");

        if (ok) StatusMessage = "The pipeline was rebuilt from the saved settings.";

        return ok;
    }

    public bool SaveFrame(string iPath)
    {
        if (_handle == IntPtr.Zero) return false;

        bool ok = Report(NativeEngine.FeEngine_SaveFrame(_handle, iPath), "The frame could not be saved.");

        if (ok) StatusMessage = $"Saved to {iPath}";

        return ok;
    }

    public bool SetRecording(bool iRecording, string iDirectory)
    {
        if (_handle == IntPtr.Zero) return false;

        bool ok = Report(NativeEngine.FeEngine_SetRecording(_handle, iRecording ? 1 : 0, iDirectory),
                         "Recording could not be started.");

        if (ok)
        {
            IsRecording = iRecording;
            StatusMessage = iRecording ? $"Recording into {iDirectory}" : "Recording stopped.";
        }

        return ok;
    }

    /// <summary>
    /// Binds the panels to the engine's swap chains. Called once when the live view is
    /// loaded, and again by <see cref="Poll"/> if the graphics device was rebuilt.
    /// </summary>
    public void AttachPanels(SwapChainPanel iVideoPanel, SwapChainPanel iHeadPanel)
    {
        _videoPanel = iVideoPanel;
        _headPanel = iHeadPanel;

        BindPanels();
    }

    public void DetachPanels()
    {
        _videoPanel = null;
        _headPanel = null;
        _boundGeneration = 0;
    }

    private void BindPanels()
    {
        if (_handle == IntPtr.Zero) return;

        try
        {
            if (_videoPanel is not null &&
                NativeEngine.FeEngine_CreateVideoSwapChain(_handle, out IntPtr videoChain) == NativeEngine.Status.Ok)
            {
                try
                {
                    SwapChainBinder.Attach(_videoPanel, videoChain);
                }
                finally
                {
                    System.Runtime.InteropServices.Marshal.Release(videoChain);
                }

                ResizeVideoPanel();
            }

            if (_headPanel is not null &&
                NativeEngine.FeEngine_CreateHeadSwapChain(_handle, out IntPtr headChain) == NativeEngine.Status.Ok)
            {
                try
                {
                    SwapChainBinder.Attach(_headPanel, headChain);
                }
                finally
                {
                    System.Runtime.InteropServices.Marshal.Release(headChain);
                }

                ResizeHeadPanel();
            }

            _boundGeneration = Snapshot.RendererGeneration;
        }
        catch (Exception exception)
        {
            StatusMessage = $"The rendering surfaces could not be bound: {exception.Message}";
        }
    }

    /// <summary>
    /// Tells the engine how large the video panel really is. XAML reports a logical size and
    /// composites at the panel's own scale, so the swap chain has to be sized by both.
    /// </summary>
    public void ResizeVideoPanel()
    {
        if (_handle == IntPtr.Zero || _videoPanel is null) return;

        NativeEngine.FeEngine_ResizeVideoView(_handle,
            (int)Math.Round(_videoPanel.ActualWidth),
            (int)Math.Round(_videoPanel.ActualHeight),
            _videoPanel.CompositionScaleX <= 0 ? 1.0 : _videoPanel.CompositionScaleX,
            _videoPanel.CompositionScaleY <= 0 ? 1.0 : _videoPanel.CompositionScaleY);
    }

    public void ResizeHeadPanel()
    {
        if (_handle == IntPtr.Zero || _headPanel is null) return;

        NativeEngine.FeEngine_ResizeHeadView(_handle,
            (int)Math.Round(_headPanel.ActualWidth),
            (int)Math.Round(_headPanel.ActualHeight),
            _headPanel.CompositionScaleX <= 0 ? 1.0 : _headPanel.CompositionScaleX,
            _headPanel.CompositionScaleY <= 0 ? 1.0 : _headPanel.CompositionScaleY);
    }

    private void Poll()
    {
        if (_handle == IntPtr.Zero) return;

        if (NativeEngine.FeEngine_GetSnapshot(_handle, out NativeEngine.Snapshot snapshot) != NativeEngine.Status.Ok)
        {
            return;
        }

        Snapshot = snapshot;

        // The device was lost and rebuilt, so the chains the panels hold belong to a device
        // that no longer exists
        if (_boundGeneration != 0 && snapshot.RendererGeneration != _boundGeneration)
        {
            BindPanels();
        }

        UpdateFaces(snapshot);
        UpdateStages(snapshot);

        LatencyHistory.Push(snapshot.LatencyMs);
        PipelineFpsHistory.Push(snapshot.PipelineFps);
        CaptureFpsHistory.Push(snapshot.CaptureFps);

        OnPropertyChanged(nameof(Snapshot));
        OnPropertyChanged(nameof(IsRunning));
        OnPropertyChanged(nameof(IsPaused));
        OnPropertyChanged(nameof(PipelineDrawsOverlay));

        Updated?.Invoke(this, EventArgs.Empty);
    }

    private void UpdateFaces(in NativeEngine.Snapshot iSnapshot)
    {
        int count = Math.Clamp(iSnapshot.FaceCount, 0, NativeEngine.MaxFaces);

        // Updated in place rather than rebuilt: a collection that is cleared and refilled
        // twenty times a second makes the panels flicker and loses the selection
        while (Faces.Count > count) Faces.RemoveAt(Faces.Count - 1);
        while (Faces.Count < count) Faces.Add(new FaceStatus());

        for (int i = 0; i < count; i++)
        {
            Faces[i].Update(iSnapshot.Faces[i]);
        }
    }

    private void UpdateStages(in NativeEngine.Snapshot iSnapshot)
    {
        int count = Math.Clamp(iSnapshot.StageCount, 0, NativeEngine.MaxStages);

        // A stage is addressed by index, and the profiler discovers its stages as they first
        // run, so an index does not mean the same stage for the whole session. The engine
        // says when that changed; until it does, only the timings are read.
        if (_stageLayoutVersion != iSnapshot.StageLayoutVersion)
        {
            _stageLayoutVersion = iSnapshot.StageLayoutVersion;

            Stages.Clear();

            for (int i = 0; i < count; i++)
            {
                int index = i;

                Stages.Add(new StageTiming(NativeEngine.ReadString((buffer, capacity) =>
                    NativeEngine.FeEngine_GetStageName(_handle, index, buffer, capacity), 128)));
            }
        }

        // The count and the version are read from the same snapshot, but the names were read
        // after it, so a layout that changed in between leaves the two disagreeing for a tick
        count = Math.Min(count, Stages.Count);

        double slowest = 0.0;

        for (int i = 0; i < count; i++) slowest = Math.Max(slowest, iSnapshot.StageMs[i]);

        for (int i = 0; i < count; i++)
        {
            Stages[i].Update(iSnapshot.StageMs[i], slowest);
        }
    }

    private void PushOverlayOptions()
    {
        if (_handle != IntPtr.Zero) NativeEngine.FeEngine_SetOverlayOptions(_handle, in _overlay);
    }

    private void PushHeadOptions()
    {
        if (_handle != IntPtr.Zero) NativeEngine.FeEngine_SetHeadOptions(_handle, in _head);
    }

    private string ReadLastError()
    {
        if (_handle == IntPtr.Zero) return string.Empty;

        return NativeEngine.ReadString((buffer, capacity) =>
            NativeEngine.FeEngine_GetLastError(_handle, buffer, capacity), 1024);
    }

    private bool Report(NativeEngine.Status iStatus, string iFallback)
    {
        if (iStatus == NativeEngine.Status.Ok)
        {
            StatusMessage = string.Empty;
            return true;
        }

        string message = ReadLastError();
        StatusMessage = message.Length > 0 ? message : iFallback;

        return false;
    }

    private void OnPropertyChanged([CallerMemberName] string? iName = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(iName));
    }

    private void Set<T>(ref T ioField, T iValue, [CallerMemberName] string? iName = null)
    {
        if (EqualityComparer<T>.Default.Equals(ioField, iValue)) return;

        ioField = iValue;
        OnPropertyChanged(iName);
    }

    public void Dispose()
    {
        _timer?.Stop();

        if (_handle == IntPtr.Zero) return;

        // The panels have to let go before the chains do, or the compositor is left holding
        // a surface of a device that is being destroyed
        if (_videoPanel is not null) TryDetach(_videoPanel);
        if (_headPanel is not null) TryDetach(_headPanel);

        NativeEngine.FeEngine_Destroy(_handle);
        _handle = IntPtr.Zero;
    }

    private static void TryDetach(SwapChainPanel iPanel)
    {
        try
        {
            SwapChainBinder.Attach(iPanel, IntPtr.Zero);
        }
        catch (Exception)
        {
            // A panel that is already gone cannot be unbound, and there is nothing left to
            // do about it on the way out
        }
    }
}
