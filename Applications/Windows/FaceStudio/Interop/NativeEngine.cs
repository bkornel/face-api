using System;
using System.Runtime.InteropServices;

namespace FaceStudio.Interop;

/// <summary>
/// The managed side of FaceEngine.h.
///
/// Every declaration here mirrors one in that header field by field. Nothing is interpreted
/// on the way through: the wrapping into something the UI can bind to happens one layer up,
/// in <see cref="Services.EngineService"/>, so that a change to the native contract shows up
/// as a compile error in exactly one file.
/// </summary>
public static class NativeEngine
{
    private const string Library = "FaceEngine.dll";

    /// <summary>Faces a snapshot can report. Mirrors FE_MAX_FACES.</summary>
    public const int MaxFaces = 8;

    /// <summary>Pipeline stages a snapshot can report. Mirrors FE_MAX_STAGES.</summary>
    public const int MaxStages = 16;

    public enum Status
    {
        Ok = 0,
        Failed = 1,
        InvalidArgument = 2,
        NotInitialized = 3,
        SourceFailed = 4,
        NoData = 5,
        Unsupported = 6
    }

    public enum SourceKind
    {
        None = 0,
        Camera = 1,
        File = 2
    }

    /// <summary>Mirrors face::TrackStatus, which is what the pipeline reports.</summary>
    public enum TrackState
    {
        Detected = 0,
        Tracked = 1,
        Inactive = 2
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Face
    {
        public double AgeSeconds;

        public double YawDeg;
        public double PitchDeg;
        public double RollDeg;

        public double PosX;
        public double PosY;
        public double PosZ;

        public double RectX;
        public double RectY;
        public double RectWidth;
        public double RectHeight;

        public double OpenMouth;
        public double OpenEyeLeft;
        public double OpenEyeRight;
        public double BrowRaise;
        public double Smile;

        public int UserId;
        public int State;
        public int HasPose;
        public int Reserved;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Snapshot
    {
        public long TimestampMs;

        public ulong FramesCaptured;
        public ulong FramesProcessed;
        public ulong FramesDropped;
        public ulong FramesRendered;

        public double CaptureFps;
        public double PipelineFps;
        public double RenderFps;

        public double LatencyMs;
        public double LatencyMinMs;
        public double LatencyMaxMs;
        public double LatencyAvgMs;
        public double LatencyP95Ms;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = MaxStages)]
        public double[] StageMs;

        public int FrameId;
        public int SourceWidth;
        public int SourceHeight;
        public int FaceCount;
        public int QueueDepth;
        public int StageCount;
        public int IsRunning;
        public int IsPaused;
        public int SourceKindValue;
        public int PipelineDrawsOverlay;
        public int RendererGeneration;
        public int StageLayoutVersion;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = MaxFaces)]
        public Face[] Faces;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct OverlayOptions
    {
        public double GlowStrength;
        public double PointSize;

        public int ShowMesh;
        public int ShowPoints;
        public int ShowRect;
        public int ShowPoseBox;
        public int ShowAxes;
        public int ShowLabels;
        public int Mirror;
        public int Reserved;

        public static OverlayOptions CreateDefault() => new()
        {
            GlowStrength = 1.0,
            PointSize = 1.6,
            ShowMesh = 1,
            ShowPoints = 1,
            ShowRect = 1,
            ShowPoseBox = 1,
            ShowAxes = 1,
            ShowLabels = 1,
            Mirror = 1
        };
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct HeadOptions
    {
        public double OrbitYawDeg;
        public double OrbitPitchDeg;
        public double Distance;
        public double PoseFollow;
        public double Expression;

        public int ShowWireframe;
        public int ShowLandmarks;
        public int ShowEyes;
        public int IdleSpin;

        public static HeadOptions CreateDefault() => new()
        {
            OrbitYawDeg = 0.0,
            OrbitPitchDeg = 0.0,
            Distance = 3.1,
            PoseFollow = 1.0,
            Expression = 1.2,
            ShowWireframe = 0,
            ShowLandmarks = 0,
            ShowEyes = 1,
            IdleSpin = 1
        };
    }

    // Classic DllImport rather than the LibraryImport source generator: the snapshot carries
    // fixed-size arrays that the generator cannot lay out, and having one style for the whole
    // boundary is worth more here than the marginally cheaper stubs of a mixed one.
    [DllImport(Library, CallingConvention = CallingConvention.StdCall)]
    public static extern void FeEngine_GetStructSizes(out int oSnapshotSize, out int oFaceSize);

    [DllImport(Library, CallingConvention = CallingConvention.StdCall)]
    public static extern int FeEngine_GetAccentCount();

    [DllImport(Library, CallingConvention = CallingConvention.StdCall)]
    public static extern void FeEngine_GetAccentColor(int iIndex, out double oR, out double oG, out double oB);

    [DllImport(Library, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
    public static extern Status FeEngine_Create(string iWorkingDirectory, out IntPtr oEngine);

    [DllImport(Library, CallingConvention = CallingConvention.StdCall)]
    public static extern void FeEngine_Destroy(IntPtr iEngine);

    [DllImport(Library, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
    public static extern Status FeEngine_GetLastError(IntPtr iEngine, [Out] char[] oBuffer, int iCapacity);

    [DllImport(Library, CallingConvention = CallingConvention.StdCall)]
    public static extern int FeEngine_EnumerateCameras(IntPtr iEngine, int iMaxProbe);

    [DllImport(Library, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
    public static extern Status FeEngine_GetCameraName(IntPtr iEngine, int iIndex, [Out] char[] oBuffer, int iCapacity);

    [DllImport(Library, CallingConvention = CallingConvention.StdCall)]
    public static extern Status FeEngine_OpenCamera(IntPtr iEngine, int iIndex, int iWidth, int iHeight, double iFps);

    [DllImport(Library, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
    public static extern Status FeEngine_OpenFile(IntPtr iEngine, string iPath);

    [DllImport(Library, CallingConvention = CallingConvention.StdCall)]
    public static extern Status FeEngine_CloseSource(IntPtr iEngine);

    [DllImport(Library, CallingConvention = CallingConvention.StdCall)]
    public static extern void FeEngine_SetPaused(IntPtr iEngine, int iPaused);

    [DllImport(Library, CallingConvention = CallingConvention.StdCall)]
    public static extern void FeEngine_SetLooping(IntPtr iEngine, int iLooping);

    [DllImport(Library, CallingConvention = CallingConvention.StdCall)]
    public static extern Status FeEngine_CreateVideoSwapChain(IntPtr iEngine, out IntPtr oSwapChain);

    [DllImport(Library, CallingConvention = CallingConvention.StdCall)]
    public static extern Status FeEngine_ResizeVideoView(IntPtr iEngine, int iWidth, int iHeight, double iScaleX, double iScaleY);

    [DllImport(Library, CallingConvention = CallingConvention.StdCall)]
    public static extern Status FeEngine_CreateHeadSwapChain(IntPtr iEngine, out IntPtr oSwapChain);

    [DllImport(Library, CallingConvention = CallingConvention.StdCall)]
    public static extern Status FeEngine_ResizeHeadView(IntPtr iEngine, int iWidth, int iHeight, double iScaleX, double iScaleY);

    [DllImport(Library, CallingConvention = CallingConvention.StdCall)]
    public static extern void FeEngine_SetOverlayOptions(IntPtr iEngine, in OverlayOptions iOptions);

    [DllImport(Library, CallingConvention = CallingConvention.StdCall)]
    public static extern void FeEngine_SetHeadOptions(IntPtr iEngine, in HeadOptions iOptions);

    [DllImport(Library, CallingConvention = CallingConvention.StdCall)]
    public static extern Status FeEngine_ViewToFrame(IntPtr iEngine, double iViewX, double iViewY, out double oFrameX, out double oFrameY);

    [DllImport(Library, CallingConvention = CallingConvention.StdCall)]
    public static extern Status FeEngine_GetSnapshot(IntPtr iEngine, out Snapshot oSnapshot);

    [DllImport(Library, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
    public static extern Status FeEngine_GetStageName(IntPtr iEngine, int iIndex, [Out] char[] oBuffer, int iCapacity);

    [DllImport(Library, CallingConvention = CallingConvention.StdCall)]
    public static extern Status FeEngine_ReloadPipeline(IntPtr iEngine);

    [DllImport(Library, CallingConvention = CallingConvention.StdCall)]
    public static extern void FeEngine_ClearUsers(IntPtr iEngine);

    [DllImport(Library, CallingConvention = CallingConvention.StdCall)]
    public static extern void FeEngine_ForceDetection(IntPtr iEngine);

    [DllImport(Library, CallingConvention = CallingConvention.StdCall)]
    public static extern void FeEngine_SetVerbose(IntPtr iEngine, int iVerbose);

    [DllImport(Library, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
    public static extern Status FeEngine_SaveFrame(IntPtr iEngine, string iPath);

    [DllImport(Library, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
    public static extern Status FeEngine_SetRecording(IntPtr iEngine, int iRecording, string iDirectory);

    /// <summary>
    /// Reads one of the string-returning calls into a managed string.
    /// </summary>
    public static string ReadString(Func<char[], int, Status> iCall, int iCapacity = 512)
    {
        var buffer = new char[iCapacity];

        if (iCall(buffer, buffer.Length) != Status.Ok && buffer[0] == '\0')
        {
            return string.Empty;
        }

        int length = Array.IndexOf(buffer, '\0');
        return new string(buffer, 0, length < 0 ? buffer.Length : length);
    }
}
