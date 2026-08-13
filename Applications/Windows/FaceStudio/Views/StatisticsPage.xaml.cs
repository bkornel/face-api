using System;

using FaceStudio.Interop;
using FaceStudio.Services;

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace FaceStudio.Views;

/// <summary>
/// How the pipeline is doing, over time rather than at this instant.
///
/// Every number here comes from the same snapshot the live view reads; what this page adds
/// is the history, which is where a stall or a slow drift shows up and a single reading
/// never does.
/// </summary>
public sealed partial class StatisticsPage : Page
{
    private readonly EngineService _engine = App.Engine;

    public StatisticsPage()
    {
        InitializeComponent();

        StagesList.ItemsSource = _engine.Stages;

        Loaded += (_, _) =>
        {
            _engine.Updated += OnEngineUpdated;
            Refresh();
        };

        Unloaded += (_, _) => _engine.Updated -= OnEngineUpdated;
    }

    private void OnEngineUpdated(object? iSender, EventArgs iArgs) => Refresh();

    private void Refresh()
    {
        NativeEngine.Snapshot snapshot = _engine.Snapshot;

        CaptureFpsText.Text = $"{snapshot.CaptureFps:0.0}";
        PipelineFpsText.Text = $"{snapshot.PipelineFps:0.0}";
        RenderFpsText.Text = $"{snapshot.RenderFps:0.0}";

        LatencyAvgText.Text = $"{snapshot.LatencyAvgMs:0.0}";
        LatencyRangeText.Text = $"{snapshot.LatencyMinMs:0.0} – {snapshot.LatencyMaxMs:0.0} ms observed";
        LatencyPeakText.Text = $"peak {_engine.LatencyHistory.Maximum:0.0} ms";

        CapturedText.Text = snapshot.FramesCaptured.ToString("N0");
        ProcessedText.Text = snapshot.FramesProcessed.ToString("N0");
        RenderedText.Text = snapshot.FramesRendered.ToString("N0");
        DroppedText.Text = snapshot.FramesDropped.ToString("N0");
        QueueText.Text = snapshot.QueueDepth.ToString();

        FrameIdText.Text = snapshot.FrameId.ToString("N0");
        SourceSizeText.Text = snapshot.SourceWidth > 0
            ? $"{snapshot.SourceWidth} × {snapshot.SourceHeight}"
            : "—";

        LatencyP95Text.Text = $"{snapshot.LatencyP95Ms:0.0} ms";
        LatencyMaxText.Text = $"{snapshot.LatencyMaxMs:0.0} ms";
        FaceCountText.Text = snapshot.FaceCount.ToString();

        LatencyChart.Update(_engine.LatencyHistory);

        // Both throughput lines are drawn against the same ceiling, so that the distance
        // between them means what the caption underneath says it means
        double ceiling = Math.Max(5.0, Math.Max(_engine.CaptureFpsHistory.Maximum, _engine.PipelineFpsHistory.Maximum));

        CaptureChart.FixedMaximum = ceiling;
        PipelineChart.FixedMaximum = ceiling;

        CaptureChart.Update(_engine.CaptureFpsHistory);
        PipelineChart.Update(_engine.PipelineFpsHistory);

        bool hasStages = _engine.Stages.Count > 0;

        NoStagesText.Visibility = hasStages ? Visibility.Collapsed : Visibility.Visible;
        StagesList.Visibility = hasStages ? Visibility.Visible : Visibility.Collapsed;
    }
}
