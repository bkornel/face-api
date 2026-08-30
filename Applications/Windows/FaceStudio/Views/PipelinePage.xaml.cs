using System;
using System.ComponentModel;
using System.IO;
using System.Linq;

using FaceStudio.Services;

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace FaceStudio.Views;

/// <summary>
/// The settings editor: the pipeline's own settings.json, with the ranges and the
/// explanations the file itself cannot carry.
///
/// Saving writes the file and asks the engine to build the graph again, which is what makes
/// this an editor rather than a viewer - a change is visible in the live view a moment
/// later, without restarting anything.
/// </summary>
public sealed partial class PipelinePage : Page
{
    private readonly EngineService _engine = App.Engine;

    private PipelineSettings _settings;

    private bool _loading;

    public PipelinePage()
    {
        InitializeComponent();

        _settings = PipelineSettings.Load(SettingsPath);

        PathText.Text = SettingsPath;

        Bind();
    }

    private static string SettingsPath => Path.Combine(App.WorkingDirectory, "settings.json");

    private void Bind()
    {
        _loading = true;

        GeneralList.ItemsSource = _settings.General;
        ModulesList.ItemsSource = _settings.Modules;

        RendererChoice.SelectedIndex = _settings.Renderer == OverlayRenderer.Pipeline ? 1 : 0;

        foreach (ParameterSetting parameter in AllParameters())
        {
            parameter.PropertyChanged -= OnParameterChanged;
            parameter.PropertyChanged += OnParameterChanged;
        }

        _loading = false;

        UpdateDirtyState();
    }

    private System.Collections.Generic.IEnumerable<ParameterSetting> AllParameters()
    {
        return _settings.General.Concat(_settings.Modules.SelectMany(module => module.Parameters));
    }

    private void OnParameterChanged(object? iSender, PropertyChangedEventArgs iArgs)
    {
        if (iArgs.PropertyName == nameof(ParameterSetting.IsModified)) UpdateDirtyState();
    }

    private void OnRendererChanged(object iSender, SelectionChangedEventArgs iArgs)
    {
        if (_loading) return;

        _settings.Renderer = RendererChoice.SelectedIndex == 1
            ? OverlayRenderer.Pipeline
            : OverlayRenderer.Application;

        UpdateDirtyState();
    }

    private void UpdateDirtyState()
    {
        DirtyText.Visibility = _settings.IsModified ? Visibility.Visible : Visibility.Collapsed;
    }

    private void OnRevertClicked(object iSender, RoutedEventArgs iArgs)
    {
        _settings.Revert();
        Bind();
    }

    private void OnApplyClicked(object iSender, RoutedEventArgs iArgs)
    {
        ApplyButton.IsEnabled = false;

        try
        {
            if (!_settings.Save(out string error))
            {
                ShowDialog("The settings could not be saved", error);
                return;
            }

            // Rebuilding takes the graph down and up again; the source is left running and
            // the frames that arrive in between are dropped by the queue
            _engine.ReloadPipeline();

            // Read back from disk, so that what is on screen is what is now on disk rather
            // than what was typed into it
            _settings = PipelineSettings.Load(SettingsPath);
            Bind();
        }
        finally
        {
            ApplyButton.IsEnabled = true;
        }
    }

    private async void ShowDialog(string iTitle, string iMessage)
    {
        var dialog = new ContentDialog
        {
            XamlRoot = XamlRoot,
            Title = iTitle,
            Content = iMessage,
            CloseButtonText = "Close"
        };

        await dialog.ShowAsync();
    }
}
