using System;
using System.IO;

using FaceStudio.Interop;
using FaceStudio.Services;
using FaceStudio.Views;

using Microsoft.UI;
using Microsoft.UI.Input;
using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;

using Windows.Graphics;
using Windows.System;

namespace FaceStudio;

/// <summary>
/// The shell: the title bar, the sidebar and whatever the engine last had to say.
///
/// The transport lives in the title bar rather than on the Live page, because pausing and
/// recording apply wherever the user happens to be looking.
/// </summary>
public sealed partial class MainWindow : Window
{
    private readonly EngineService _engine = App.Engine;

    private bool _statusPinned;

    public MainWindow()
    {
        InitializeComponent();

        Title = "Face Studio";

        ExtendsContentIntoTitleBar = true;
        SetTitleBar(DragRegion);

        // Mica is the whole reason the chrome can be this quiet. Where it is not available -
        // Windows 10 - acrylic gives the same depth, and the layout does not change either way.
        SystemBackdrop = Microsoft.UI.Composition.SystemBackdrops.MicaController.IsSupported()
            ? new MicaBackdrop { Kind = Microsoft.UI.Composition.SystemBackdrops.MicaKind.BaseAlt }
            : new DesktopAcrylicBackdrop();

        AppWindow.Resize(new SizeInt32(1520, 960));

        if (AppWindow.Presenter is OverlappedPresenter presenter)
        {
            presenter.PreferredMinimumWidth = 1080;
            presenter.PreferredMinimumHeight = 720;
        }

        _engine.PropertyChanged += OnEnginePropertyChanged;
        _engine.Updated += OnEngineUpdated;

        RootGrid.KeyDown += OnKeyDown;

        ContentFrame.Navigate(typeof(LivePage));

        if (!_engine.IsAvailable) ShowStatus(_engine.StatusMessage, isProblem: true, pinned: true);
    }

    private void OnNavigationSelectionChanged(NavigationView iSender, NavigationViewSelectionChangedEventArgs iArgs)
    {
        if (iArgs.SelectedItem is not NavigationViewItem { Tag: string tag }) return;

        Type page = tag switch
        {
            "statistics" => typeof(StatisticsPage),
            "settings" => typeof(PipelinePage),
            _ => typeof(LivePage)
        };

        if (ContentFrame.CurrentSourcePageType != page) ContentFrame.Navigate(page);
    }

    private void OnEngineUpdated(object? iSender, EventArgs iArgs)
    {
        NativeEngine.Snapshot snapshot = _engine.Snapshot;

        bool running = snapshot.IsRunning != 0;
        bool paused = snapshot.IsPaused != 0;

        SourceText.Text = running
            ? $"{snapshot.SourceWidth} × {snapshot.SourceHeight}  ·  {snapshot.PipelineFps:0.0} fps"
            : "No source";

        RunIndicator.Fill = running
            ? (Brush)Application.Current.Resources[paused ? "WarningBrush" : "PositiveBrush"]
            : (Brush)Application.Current.Resources["TextTertiaryBrush"];

        PauseIcon.Glyph = paused ? "" : "";
        PauseButton.IsEnabled = running;

        RecordIcon.Foreground = _engine.IsRecording
            ? (Brush)Application.Current.Resources["NegativeBrush"]
            : (Brush)Application.Current.Resources["TextPrimaryBrush"];
    }

    private void OnEnginePropertyChanged(object? iSender, System.ComponentModel.PropertyChangedEventArgs iArgs)
    {
        if (iArgs.PropertyName != nameof(EngineService.StatusMessage)) return;

        if (string.IsNullOrEmpty(_engine.StatusMessage))
        {
            if (!_statusPinned) StatusBar.Visibility = Visibility.Collapsed;
            return;
        }

        ShowStatus(_engine.StatusMessage, isProblem: false, pinned: false);
    }

    private void ShowStatus(string iMessage, bool isProblem, bool pinned)
    {
        if (string.IsNullOrEmpty(iMessage)) return;

        StatusText.Text = iMessage;
        StatusIcon.Glyph = isProblem ? "" : "";

        StatusIcon.Foreground = (Brush)Application.Current.Resources[isProblem ? "WarningBrush" : "TextSecondaryBrush"];

        StatusBar.Visibility = Visibility.Visible;
        _statusPinned = pinned;
    }

    private void OnDismissStatusClicked(object iSender, RoutedEventArgs iArgs)
    {
        StatusBar.Visibility = Visibility.Collapsed;
        _statusPinned = false;
    }

    private void OnPauseClicked(object iSender, RoutedEventArgs iArgs)
    {
        _engine.SetPaused(!_engine.IsPaused);
    }

    private async void OnSnapshotClicked(object iSender, RoutedEventArgs iArgs)
    {
        string directory = Path.Combine(App.WorkingDirectory, "..", "configurations", "output");

        try
        {
            Directory.CreateDirectory(directory);
        }
        catch (Exception exception)
        {
            ShowStatus($"The output directory could not be created: {exception.Message}", isProblem: true, pinned: false);
            return;
        }

        string path = Path.GetFullPath(Path.Combine(directory,
            $"studio_{DateTime.Now:yyyyMMdd_HHmmss}_{_engine.Snapshot.FrameId}.png"));

        // The copy is taken by the render thread between drawing and presenting, so the call
        // waits for a frame; off the UI thread, so the window does not stop for it
        bool saved = await System.Threading.Tasks.Task.Run(() => _engine.SaveFrame(path));

        if (!saved) ShowStatus(_engine.StatusMessage, isProblem: true, pinned: false);
    }

    private void OnRecordClicked(object iSender, RoutedEventArgs iArgs)
    {
        string directory = Path.GetFullPath(Path.Combine(App.WorkingDirectory, "..", "configurations", "output"));

        try
        {
            Directory.CreateDirectory(directory);
        }
        catch (Exception exception)
        {
            ShowStatus($"The output directory could not be created: {exception.Message}", isProblem: true, pinned: false);
            return;
        }

        _engine.SetRecording(!_engine.IsRecording, directory);
    }

    private void OnKeyDown(object iSender, KeyRoutedEventArgs iArgs)
    {
        bool control = InputKeyboardSource
            .GetKeyStateForCurrentThread(VirtualKey.Control)
            .HasFlag(Windows.UI.Core.CoreVirtualKeyStates.Down);

        switch (iArgs.Key)
        {
            case VirtualKey.Space:
                _engine.SetPaused(!_engine.IsPaused);
                iArgs.Handled = true;
                break;

            case VirtualKey.D when control:
                _engine.ForceDetection();
                iArgs.Handled = true;
                break;

            case VirtualKey.S when control:
                OnSnapshotClicked(this, new RoutedEventArgs());
                iArgs.Handled = true;
                break;

            case VirtualKey.K when control:
                _engine.ClearUsers();
                iArgs.Handled = true;
                break;
        }
    }
}
