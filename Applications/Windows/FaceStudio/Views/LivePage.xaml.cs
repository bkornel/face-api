using System;

using FaceStudio.Interop;
using FaceStudio.Services;

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Navigation;

using Windows.Storage.Pickers;

namespace FaceStudio.Views;

/// <summary>
/// The camera, the artificial head and the inspector.
///
/// The two views on this page are composited by the system from the engine's swap chains,
/// so nothing here touches a frame. What this class does is bind the panels, keep the
/// controls and the engine's options in step, and copy a handful of numbers into the labels
/// on every tick.
/// </summary>
public sealed partial class LivePage : Page
{
    private readonly EngineService _engine = App.Engine;
    private readonly AppPreferences _preferences = App.Preferences;

    /// <summary>
    /// Set while the controls are being filled in, so that doing so does not read back as
    /// the user changing them.
    ///
    /// It starts true and is only cleared once the page has been loaded and the controls
    /// carry the stored preferences. XAML raises ValueChanged on a slider as it parses the
    /// attributes - assigning Minimum clamps Value, which raises it before the rest of the
    /// page exists - and a handler that ran then would be reading half a page.
    /// </summary>
    private bool _loading = true;

    private bool _orbiting;
    private Windows.Foundation.Point _orbitOrigin;
    private double _orbitYawAtPress;
    private double _orbitPitchAtPress;

    public LivePage()
    {
        InitializeComponent();

        FacesList.ItemsSource = _engine.Faces;

        Loaded += OnLoaded;
        Unloaded += OnUnloaded;

        VideoPanel.SizeChanged += (_, _) => _engine.ResizeVideoPanel();
        VideoPanel.CompositionScaleChanged += (_, _) => _engine.ResizeVideoPanel();

        HeadPanel.SizeChanged += (_, _) => _engine.ResizeHeadPanel();
        HeadPanel.CompositionScaleChanged += (_, _) => _engine.ResizeHeadPanel();

        HeadPanel.PointerPressed += OnHeadPointerPressed;
        HeadPanel.PointerMoved += OnHeadPointerMoved;
        HeadPanel.PointerReleased += OnHeadPointerReleased;
        HeadPanel.PointerCaptureLost += OnHeadPointerReleased;
    }

    private void OnLoaded(object iSender, RoutedEventArgs iArgs)
    {
        LoadControlsFromPreferences();

        if (!_engine.IsAvailable) return;

        _engine.AttachPanels(VideoPanel, HeadPanel);
        _engine.Updated += OnEngineUpdated;

        FillCameraList();

        // Coming back to this page with a camera already running: show which one, without
        // reopening it. Selecting it for real is what OnCameraSelectionChanged is for.
        if (_engine.Snapshot.SourceKindValue == (int)NativeEngine.SourceKind.Camera)
        {
            _loading = true;

            CameraCombo.SelectedIndex = _preferences.LastCameraIndex >= 0 && _preferences.LastCameraIndex < _engine.Cameras.Count
                ? _preferences.LastCameraIndex
                : -1;

            _loading = false;
            return;
        }

        // Reopen what was last used, so that starting the application is one step rather
        // than three. A camera that has since been unplugged simply fails and says so.
        if (_preferences.AutoStart && _engine.Snapshot.IsRunning == 0)
        {
            if (_preferences.LastCameraIndex >= 0 && _preferences.LastCameraIndex < _engine.Cameras.Count)
            {
                CameraCombo.SelectedIndex = _preferences.LastCameraIndex;
            }
            else if (_engine.Cameras.Count > 0)
            {
                CameraCombo.SelectedIndex = 0;
            }
        }
    }

    private void OnUnloaded(object iSender, RoutedEventArgs iArgs)
    {
        _engine.Updated -= OnEngineUpdated;

        // The panels are going away with the page, and the swap chains must not be left
        // bound to visuals that are being torn down
        _engine.DetachPanels();
    }

    private void LoadControlsFromPreferences()
    {
        _loading = true;

        MirrorCheck.IsChecked = _preferences.MirrorView;
        MeshCheck.IsChecked = _preferences.ShowMesh;
        PointsCheck.IsChecked = _preferences.ShowPoints;
        RectCheck.IsChecked = _preferences.ShowRect;
        PoseBoxCheck.IsChecked = _preferences.ShowPoseBox;
        AxesCheck.IsChecked = _preferences.ShowAxes;
        LabelsCheck.IsChecked = _preferences.ShowLabels;
        GlowSlider.Value = _preferences.GlowStrength;

        FollowPoseCheck.IsChecked = _preferences.HeadFollowsPose;
        WireframeCheck.IsChecked = _preferences.HeadWireframe;
        LandmarksCheck.IsChecked = _preferences.HeadLandmarks;
        EyesCheck.IsChecked = _preferences.HeadEyes;
        ExpressionSlider.Value = _preferences.HeadExpression;
        DistanceSlider.Value = _preferences.HeadDistance;

        LoopSwitch.IsOn = _preferences.LoopVideoFiles;

        GlowValueText.Text = $"{GlowSlider.Value:0.00}";
        ExpressionValueText.Text = $"{ExpressionSlider.Value:0.00}";
        DistanceValueText.Text = $"{DistanceSlider.Value:0.00}";

        _loading = false;
    }

    private void FillCameraList()
    {
        _loading = true;

        CameraCombo.Items.Clear();

        foreach (string camera in _engine.Cameras) CameraCombo.Items.Add(camera);

        CameraCombo.IsEnabled = _engine.Cameras.Count > 0;

        // "No camera found" is only true when none were; with a list and no selection it is
        // an invitation, not a report
        CameraCombo.PlaceholderText = _engine.Cameras.Count > 0 ? "Choose a camera" : "No camera found";

        _loading = false;
    }

    private void OnEngineUpdated(object? iSender, EventArgs iArgs)
    {
        NativeEngine.Snapshot snapshot = _engine.Snapshot;

        PipelineFpsText.Text = $"{snapshot.PipelineFps:0.0}";
        LatencyText.Text = $"{snapshot.LatencyMs:0.0}";
        FaceCountText.Text = snapshot.FaceCount.ToString();
        QueueText.Text = snapshot.QueueDepth.ToString();

        bool running = snapshot.IsRunning != 0;

        EmptyOverlay.Visibility = running ? Visibility.Collapsed : Visibility.Visible;
        NoFacesText.Visibility = _engine.Faces.Count == 0 ? Visibility.Visible : Visibility.Collapsed;

        OverlayOwnerBadge.Visibility = _engine.PipelineDrawsOverlay ? Visibility.Visible : Visibility.Collapsed;

        // The overlay switches drive this application's renderer, which is not the one
        // drawing when the graph is wired to the pipeline's Visualizer
        bool overlayIsOurs = !_engine.PipelineDrawsOverlay;

        foreach (UIElement child in OverlayToggles.Children)
        {
            if (child is Control control) control.IsEnabled = overlayIsOurs;
        }

        GlowSlider.IsEnabled = overlayIsOurs;
    }

    private void OnCameraSelectionChanged(object iSender, SelectionChangedEventArgs iArgs)
    {
        if (_loading || CameraCombo.SelectedIndex < 0) return;

        if (_engine.OpenCamera(CameraCombo.SelectedIndex))
        {
            _preferences.LastCameraIndex = CameraCombo.SelectedIndex;
            _preferences.Save();
        }
    }

    private void OnRefreshCamerasClicked(object iSender, RoutedEventArgs iArgs)
    {
        _engine.RefreshCameras();
        FillCameraList();
    }

    private async void OnOpenFileClicked(object iSender, RoutedEventArgs iArgs)
    {
        var picker = new FileOpenPicker
        {
            SuggestedStartLocation = PickerLocationId.VideosLibrary,
            ViewMode = PickerViewMode.List
        };

        foreach (string extension in new[] { ".avi", ".mp4", ".mov", ".mkv", ".wmv", ".webm" })
        {
            picker.FileTypeFilter.Add(extension);
        }

        // An unpackaged application has no window for the picker to attach itself to, so it
        // has to be told which one to use
        if (App.MainWindow is { } window)
        {
            WinRT.Interop.InitializeWithWindow.Initialize(picker, WinRT.Interop.WindowNative.GetWindowHandle(window));
        }

        Windows.Storage.StorageFile? file = await picker.PickSingleFileAsync();

        if (file is null) return;

        if (_engine.OpenFile(file.Path))
        {
            _loading = true;
            CameraCombo.SelectedIndex = -1;
            _loading = false;
        }
    }

    private void OnStopClicked(object iSender, RoutedEventArgs iArgs)
    {
        _engine.CloseSource();

        _loading = true;
        CameraCombo.SelectedIndex = -1;
        _loading = false;
    }

    private void OnLoopToggled(object iSender, RoutedEventArgs iArgs)
    {
        if (_loading) return;

        _preferences.LoopVideoFiles = LoopSwitch.IsOn;
        _engine.SetLooping(LoopSwitch.IsOn);
    }

    private void OnForceDetectionClicked(object iSender, RoutedEventArgs iArgs)
    {
        _engine.ForceDetection();
    }

    private void OnClearUsersClicked(object iSender, RoutedEventArgs iArgs)
    {
        _engine.ClearUsers();
    }

    private void OnOverlayChanged(object iSender, RoutedEventArgs iArgs)
    {
        if (_loading) return;

        ApplyOverlayOptions();
    }

    private void OnOverlaySliderChanged(object iSender, RangeBaseValueChangedEventArgs iArgs)
    {
        if (_loading) return;

        GlowValueText.Text = $"{GlowSlider.Value:0.00}";

        ApplyOverlayOptions();
    }

    private void ApplyOverlayOptions()
    {
        _preferences.MirrorView = MirrorCheck.IsChecked == true;
        _preferences.ShowMesh = MeshCheck.IsChecked == true;
        _preferences.ShowPoints = PointsCheck.IsChecked == true;
        _preferences.ShowRect = RectCheck.IsChecked == true;
        _preferences.ShowPoseBox = PoseBoxCheck.IsChecked == true;
        _preferences.ShowAxes = AxesCheck.IsChecked == true;
        _preferences.ShowLabels = LabelsCheck.IsChecked == true;
        _preferences.GlowStrength = GlowSlider.Value;

        _engine.Overlay = _preferences.ToOverlayOptions();
    }

    private void OnHeadCheckChanged(object iSender, RoutedEventArgs iArgs)
    {
        if (_loading) return;

        ApplyHeadOptions();
    }

    private void OnHeadOptionChanged(object iSender, RangeBaseValueChangedEventArgs iArgs)
    {
        if (_loading) return;

        ExpressionValueText.Text = $"{ExpressionSlider.Value:0.00}";
        DistanceValueText.Text = $"{DistanceSlider.Value:0.00}";

        ApplyHeadOptions();
    }

    private void ApplyHeadOptions()
    {
        _preferences.HeadFollowsPose = FollowPoseCheck.IsChecked == true;
        _preferences.HeadWireframe = WireframeCheck.IsChecked == true;
        _preferences.HeadLandmarks = LandmarksCheck.IsChecked == true;
        _preferences.HeadEyes = EyesCheck.IsChecked == true;
        _preferences.HeadExpression = ExpressionSlider.Value;
        _preferences.HeadDistance = DistanceSlider.Value;

        NativeEngine.HeadOptions options = _preferences.ToHeadOptions();

        // The orbit is a gesture, not a preference: it is kept from the engine's current
        // state rather than from the file, so that dragging is not undone by a checkbox
        options.OrbitYawDeg = _engine.Head.OrbitYawDeg;
        options.OrbitPitchDeg = _engine.Head.OrbitPitchDeg;

        _engine.Head = options;
    }

    private void OnHeadPointerPressed(object iSender, PointerRoutedEventArgs iArgs)
    {
        _orbiting = true;
        _orbitOrigin = iArgs.GetCurrentPoint(HeadPanel).Position;
        _orbitYawAtPress = _engine.Head.OrbitYawDeg;
        _orbitPitchAtPress = _engine.Head.OrbitPitchDeg;

        HeadPanel.CapturePointer(iArgs.Pointer);
    }

    private void OnHeadPointerMoved(object iSender, PointerRoutedEventArgs iArgs)
    {
        if (!_orbiting) return;

        Windows.Foundation.Point position = iArgs.GetCurrentPoint(HeadPanel).Position;

        NativeEngine.HeadOptions options = _engine.Head;

        options.OrbitYawDeg = _orbitYawAtPress + (position.X - _orbitOrigin.X) * 0.4;

        // Clamped, because a head seen from directly above stops reading as a head
        options.OrbitPitchDeg = Math.Clamp(_orbitPitchAtPress + (position.Y - _orbitOrigin.Y) * 0.3, -60.0, 60.0);

        _engine.Head = options;
    }

    private void OnHeadPointerReleased(object iSender, PointerRoutedEventArgs iArgs)
    {
        _orbiting = false;
        HeadPanel.ReleasePointerCapture(iArgs.Pointer);
    }

    private void OnResetOrbitClicked(object iSender, RoutedEventArgs iArgs)
    {
        NativeEngine.HeadOptions options = _engine.Head;

        options.OrbitYawDeg = 0.0;
        options.OrbitPitchDeg = 0.0;

        _engine.Head = options;
    }

    protected override void OnNavigatedFrom(NavigationEventArgs iArgs)
    {
        base.OnNavigatedFrom(iArgs);

        _preferences.Save();
    }
}
