using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace FaceStudio.Models;

/// <summary>
/// One stage of the pipeline and what it cost on the last frame.
///
/// The name comes from the profiler that is compiled into the API, so the list is whatever
/// that build measures rather than anything this application decides.
/// </summary>
public sealed class StageTiming : INotifyPropertyChanged
{
    private double _milliseconds;
    private double _share;

    public StageTiming(string iName)
    {
        Name = Prettify(iName);
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    public string Name { get; }

    public double Milliseconds
    {
        get => _milliseconds;
        private set
        {
            if (Set(ref _milliseconds, value)) OnPropertyChanged(nameof(MillisecondsText));
        }
    }

    /// <summary>Length of this stage's bar, as a fraction of the slowest stage.</summary>
    public double Share
    {
        get => _share;
        private set => Set(ref _share, value);
    }

    public string MillisecondsText => $"{Milliseconds:0.0} ms";

    public void Update(double iMilliseconds, double iSlowest)
    {
        Milliseconds = iMilliseconds;
        Share = iSlowest > 0.0 ? Math.Clamp(iMilliseconds / iSlowest, 0.0, 1.0) : 0.0;
    }

    /// <summary>
    /// The profiler names its stages for sorting - "1_Capture", "4_Draw" - which is exactly
    /// what a reader does not need to see.
    /// </summary>
    private static string Prettify(string iName)
    {
        if (string.IsNullOrEmpty(iName)) return "—";

        int underscore = iName.IndexOf('_');

        string trimmed = underscore > 0 && underscore < iName.Length - 1 && char.IsDigit(iName[0])
            ? iName[(underscore + 1)..]
            : iName;

        return trimmed.Replace('_', ' ');
    }

    private bool Set<T>(ref T ioField, T iValue, [CallerMemberName] string? iName = null)
    {
        if (Equals(ioField, iValue)) return false;

        ioField = iValue;
        OnPropertyChanged(iName);

        return true;
    }

    private void OnPropertyChanged([CallerMemberName] string? iName = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(iName));
    }
}
