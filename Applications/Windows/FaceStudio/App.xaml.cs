using System;
using System.IO;

using FaceStudio.Services;

using Microsoft.UI.Xaml;

namespace FaceStudio;

/// <summary>
/// The application object, and the one place the shared services are built.
///
/// There is deliberately no container and no locator: this application has exactly two
/// things with a lifetime longer than a page - the engine and the preferences - and passing
/// them around through a framework would be more machinery than either is worth.
/// </summary>
public partial class App : Application
{
    private Window? _window;

    public App()
    {
        InitializeComponent();

        // An unpackaged XAML application that throws on a UI thread disappears without a
        // window and without a message, so the reason is written down before it goes
        UnhandledException += (_, iEvent) => WriteCrashLog(iEvent.Exception);

        AppDomain.CurrentDomain.UnhandledException += (_, iEvent) =>
        {
            if (iEvent.ExceptionObject is Exception exception) WriteCrashLog(exception);
        };
    }

    /// <summary>Where the last unhandled exception was written.</summary>
    public static string CrashLogPath => Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
        "FaceStudio",
        "last-crash.log");

    private static void WriteCrashLog(Exception iException)
    {
        try
        {
            string? directory = Path.GetDirectoryName(CrashLogPath);

            if (!string.IsNullOrEmpty(directory)) Directory.CreateDirectory(directory);

            File.WriteAllText(CrashLogPath, $"{DateTime.Now:u}{Environment.NewLine}{iException}");
        }
        catch (Exception)
        {
            // Failing to record a crash is not worth crashing over
        }
    }

    /// <summary>The engine, live for as long as the application is.</summary>
    public static EngineService Engine { get; private set; } = null!;

    public static AppPreferences Preferences { get; private set; } = null!;

    /// <summary>
    /// Where the pipeline reads settings.json from. It is this application's own directory,
    /// not the one the console application uses: the two configure the graph differently,
    /// and one deployment overwriting the other's settings would be a puzzle to debug.
    /// </summary>
    public static string WorkingDirectory { get; private set; } = string.Empty;

    public static Window? MainWindow { get; private set; }

    protected override void OnLaunched(LaunchActivatedEventArgs iArgs)
    {
        Preferences = AppPreferences.Load();

        WorkingDirectory = Path.Combine(AppContext.BaseDirectory, "studio");

        Engine = new EngineService(WorkingDirectory);

        if (Engine.IsAvailable)
        {
            Engine.Overlay = Preferences.ToOverlayOptions();
            Engine.Head = Preferences.ToHeadOptions();
            Engine.SetLooping(Preferences.LoopVideoFiles);
        }

        _window = new MainWindow();
        MainWindow = _window;

        _window.Closed += (_, _) =>
        {
            Preferences.Save();
            Engine.Dispose();
        };

        _window.Activate();
    }
}
