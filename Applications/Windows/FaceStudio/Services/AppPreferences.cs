using System;
using System.IO;
using System.Text.Json;
using System.Text.Json.Serialization;

using FaceStudio.Interop;

namespace FaceStudio.Services;

/// <summary>
/// What the application remembers between runs, as opposed to what the pipeline is
/// configured with.
///
/// It lives under the user's own application data rather than next to the executable: it is
/// a preference, not a deployment, and a machine with two users should not have them fighting
/// over one file. Anything that fails to load falls back to the defaults rather than stopping
/// the application - there is nothing here worth failing to start over.
/// </summary>
public sealed class AppPreferences
{
    private static readonly JsonSerializerOptions SerializerOptions = new()
    {
        WriteIndented = true,
        DefaultIgnoreCondition = JsonIgnoreCondition.Never
    };

    public int LastCameraIndex { get; set; } = 0;

    /// <summary>Reopen the last source on startup rather than waiting to be told.</summary>
    public bool AutoStart { get; set; } = true;

    public bool LoopVideoFiles { get; set; } = true;

    public bool MirrorView { get; set; } = true;

    public bool ShowMesh { get; set; } = true;

    public bool ShowPoints { get; set; } = true;

    public bool ShowRect { get; set; } = true;

    public bool ShowPoseBox { get; set; } = true;

    public bool ShowAxes { get; set; } = true;

    public bool ShowLabels { get; set; } = true;

    public double GlowStrength { get; set; } = 1.0;

    public bool HeadFollowsPose { get; set; } = true;

    /// <summary>
    /// How much the avatar exaggerates. Above 1 by default: it is a cartoon, and a cartoon
    /// that moves exactly as much as the face does reads as a face that is barely moving.
    /// </summary>
    public double HeadExpression { get; set; } = 1.2;

    public double HeadDistance { get; set; } = 3.1;

    public bool HeadWireframe { get; set; } = false;

    public bool HeadLandmarks { get; set; } = false;

    public bool HeadEyes { get; set; } = true;

    /// <summary>"Default", "Light" or "Dark".</summary>
    public string Theme { get; set; } = "Default";

    [JsonIgnore]
    public static string FilePath => System.IO.Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
        "FaceStudio",
        "preferences.json");

    public static AppPreferences Load()
    {
        try
        {
            if (File.Exists(FilePath))
            {
                return JsonSerializer.Deserialize<AppPreferences>(File.ReadAllText(FilePath), SerializerOptions)
                       ?? new AppPreferences();
            }
        }
        catch (Exception)
        {
            // A preferences file that cannot be read is replaced by the defaults, and by the
            // next Save
        }

        return new AppPreferences();
    }

    public void Save()
    {
        try
        {
            string? directory = System.IO.Path.GetDirectoryName(FilePath);

            if (!string.IsNullOrEmpty(directory)) Directory.CreateDirectory(directory);

            File.WriteAllText(FilePath, JsonSerializer.Serialize(this, SerializerOptions));
        }
        catch (Exception)
        {
            // Not worth telling the user about: nothing they were doing has been lost
        }
    }

    public NativeEngine.OverlayOptions ToOverlayOptions() => new()
    {
        GlowStrength = GlowStrength,
        PointSize = 1.6,
        ShowMesh = ShowMesh ? 1 : 0,
        ShowPoints = ShowPoints ? 1 : 0,
        ShowRect = ShowRect ? 1 : 0,
        ShowPoseBox = ShowPoseBox ? 1 : 0,
        ShowAxes = ShowAxes ? 1 : 0,
        ShowLabels = ShowLabels ? 1 : 0,
        Mirror = MirrorView ? 1 : 0
    };

    public NativeEngine.HeadOptions ToHeadOptions() => new()
    {
        OrbitYawDeg = 0.0,
        OrbitPitchDeg = 0.0,
        Distance = HeadDistance,
        PoseFollow = HeadFollowsPose ? 1.0 : 0.0,
        Expression = HeadExpression,
        ShowWireframe = HeadWireframe ? 1 : 0,
        ShowLandmarks = HeadLandmarks ? 1 : 0,
        ShowEyes = HeadEyes ? 1 : 0,
        IdleSpin = 1
    };
}
