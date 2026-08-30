using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;

using FaceStudio.Interop;

using Microsoft.UI.Xaml.Media;

namespace FaceStudio.Models;

/// <summary>
/// One tracked face, as a panel shows it.
///
/// Instances are reused from frame to frame rather than replaced, so that the inspector
/// updates in place instead of tearing its cards down and building them again twenty times
/// a second.
/// </summary>
public sealed class FaceStatus : INotifyPropertyChanged
{
    private int _userId;
    private NativeEngine.TrackState _state;
    private bool _hasPose;
    private double _yaw;
    private double _pitch;
    private double _roll;
    private double _posX;
    private double _posY;
    private double _posZ;
    private double _width;
    private double _height;
    private double _openMouth;
    private double _openEyeLeft;
    private double _openEyeRight;
    private double _browRaise;
    private double _smile;
    private double _age;

    public event PropertyChangedEventHandler? PropertyChanged;

    public int UserId
    {
        get => _userId;
        private set
        {
            if (Set(ref _userId, value))
            {
                OnPropertyChanged(nameof(Title));
                OnPropertyChanged(nameof(Accent));
            }
        }
    }

    public NativeEngine.TrackState State
    {
        get => _state;
        private set
        {
            if (Set(ref _state, value)) OnPropertyChanged(nameof(StateText));
        }
    }

    public bool HasPose
    {
        get => _hasPose;
        private set => Set(ref _hasPose, value);
    }

    public double Yaw
    {
        get => _yaw;
        private set
        {
            if (Set(ref _yaw, value)) OnPropertyChanged(nameof(YawText));
        }
    }

    public double Pitch
    {
        get => _pitch;
        private set
        {
            if (Set(ref _pitch, value)) OnPropertyChanged(nameof(PitchText));
        }
    }

    public double Roll
    {
        get => _roll;
        private set
        {
            if (Set(ref _roll, value)) OnPropertyChanged(nameof(RollText));
        }
    }

    public double PositionX
    {
        get => _posX;
        private set
        {
            if (Set(ref _posX, value)) OnPropertyChanged(nameof(PositionText));
        }
    }

    public double PositionY
    {
        get => _posY;
        private set
        {
            if (Set(ref _posY, value)) OnPropertyChanged(nameof(PositionText));
        }
    }

    public double PositionZ
    {
        get => _posZ;
        private set
        {
            if (Set(ref _posZ, value))
            {
                OnPropertyChanged(nameof(PositionText));
                OnPropertyChanged(nameof(DistanceText));
            }
        }
    }

    public double RectWidth
    {
        get => _width;
        private set
        {
            if (Set(ref _width, value)) OnPropertyChanged(nameof(SizeText));
        }
    }

    public double RectHeight
    {
        get => _height;
        private set
        {
            if (Set(ref _height, value)) OnPropertyChanged(nameof(SizeText));
        }
    }

    public double OpenMouth
    {
        get => _openMouth;
        private set => Set(ref _openMouth, value);
    }

    public double OpenEyeLeft
    {
        get => _openEyeLeft;
        private set => Set(ref _openEyeLeft, value);
    }

    public double OpenEyeRight
    {
        get => _openEyeRight;
        private set => Set(ref _openEyeRight, value);
    }

    public double BrowRaise
    {
        get => _browRaise;
        private set => Set(ref _browRaise, value);
    }

    public double Smile
    {
        get => _smile;
        private set => Set(ref _smile, value);
    }

    public double AgeSeconds
    {
        get => _age;
        private set
        {
            if (Set(ref _age, value)) OnPropertyChanged(nameof(AgeText));
        }
    }

    public string Title => $"User {UserId}";

    public string StateText => State switch
    {
        NativeEngine.TrackState.Detected => "Detected",
        NativeEngine.TrackState.Tracked => "Tracked",
        _ => "Inactive"
    };

    public string YawText => FormatAngle(Yaw);

    public string PitchText => FormatAngle(Pitch);

    public string RollText => FormatAngle(Roll);

    public string PositionText => HasPose
        ? $"{PositionX,7:0} {PositionY,7:0} {PositionZ,7:0}"
        : "—";

    /// <summary>How far the head is from the camera, in the units the model is given in.</summary>
    public string DistanceText => HasPose ? $"{PositionZ / 10.0:0.0} cm" : "—";

    public string SizeText => RectWidth > 0 ? $"{RectWidth:0} × {RectHeight:0} px" : "—";

    public string AgeText => AgeSeconds >= 60.0
        ? $"{AgeSeconds / 60.0:0.0} min"
        : $"{AgeSeconds:0.0} s";

    /// <summary>The engine's own palette, through a cached brush: the overlay, the head and
    /// this panel colour a user from the same table.</summary>
    public SolidColorBrush Accent => Accents.BrushOf(UserId);

    public void Update(in NativeEngine.Face iFace)
    {
        UserId = iFace.UserId;
        State = (NativeEngine.TrackState)iFace.State;
        HasPose = iFace.HasPose != 0;

        Yaw = iFace.YawDeg;
        Pitch = iFace.PitchDeg;
        Roll = iFace.RollDeg;

        PositionX = iFace.PosX;
        PositionY = iFace.PosY;
        PositionZ = iFace.PosZ;

        RectWidth = iFace.RectWidth;
        RectHeight = iFace.RectHeight;

        OpenMouth = iFace.OpenMouth;
        OpenEyeLeft = iFace.OpenEyeLeft;
        OpenEyeRight = iFace.OpenEyeRight;
        BrowRaise = iFace.BrowRaise;
        Smile = iFace.Smile;

        AgeSeconds = iFace.AgeSeconds;
    }

    private static string FormatAngle(double iDegrees) => $"{iDegrees,6:0.0}°";

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
