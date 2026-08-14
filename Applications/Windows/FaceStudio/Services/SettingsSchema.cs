using System.Collections.Generic;

namespace FaceStudio.Services;

/// <summary>
/// What the settings editor knows about the pipeline's parameters.
///
/// The pipeline reads settings.json as untyped strings, so this is the only place that says
/// a threshold lives between zero and one, or that "detectionSec" is how long the tracker is
/// trusted before the detector is asked again. A key that is not listed here is still read
/// and written - it simply gets a plain text box - so adding a module does not break the
/// editor, it only makes it less helpful until a line is added below.
/// </summary>
internal static class SettingsSchema
{
    public sealed record ModuleDescriptor(
        string Name,
        string Title,
        string Description,
        IReadOnlyList<ParameterDescriptor> Parameters);

    public static readonly IReadOnlyList<ParameterDescriptor> General = new[]
    {
        new ParameterDescriptor("verbose", "Verbose logging", ParameterKind.Boolean,
            "Logs what every module is doing. Useful when a stage is misbehaving, noisy otherwise."),

        new ParameterDescriptor("video", "Write video by default", ParameterKind.Boolean,
            "Starts recording as soon as a source is opened."),

        new ParameterDescriptor("videoFourCC", "Video codec", ParameterKind.Text,
            "The four-character code of the encoder, for example XVID or MJPG."),

        new ParameterDescriptor("videoFPS", "Video frame rate", ParameterKind.Number,
            "Frame rate written into the recorded file.", 1.0, 60.0, 1.0)
    };

    public static readonly IReadOnlyList<ModuleDescriptor> Modules = new[]
    {
        new ModuleDescriptor("imageQueue", "Image queue",
            "Where pushed frames wait for the graph. It is what decouples the camera from the pipeline.",
            new[]
            {
                new ParameterDescriptor("bound", "Queue length", ParameterKind.Integer,
                    "How many frames may wait. A longer queue rides out a hiccup; it also adds that much delay before you see the result.",
                    1.0, 60.0, 1.0),

                new ParameterDescriptor("samplingFPS", "Sampling rate", ParameterKind.Number,
                    "The rate frames are taken from the source at. Below the camera's own rate, the extra frames are skipped rather than queued.",
                    1.0, 120.0, 1.0),

                new ParameterDescriptor("thresholdMS", "Stale after", ParameterKind.Integer,
                    "A frame older than this is dropped rather than processed, so the pipeline catches up instead of falling further behind.",
                    0.0, 1000.0, 10.0)
            }),

        new ModuleDescriptor("faceDetection", "Face detection",
            "Finds faces from scratch with the YuNet network. It runs rarely; the tracker carries the faces between its runs.",
            new[]
            {
                new ParameterDescriptor("detectionSec", "Re-detect every", ParameterKind.Number,
                    "How long the tracker is trusted before the detector is run again. Lower finds a new face sooner and costs more.",
                    0.0, 60.0, 0.5),

                new ParameterDescriptor("scoreThreshold", "Confidence threshold", ParameterKind.Number,
                    "How sure the network has to be. Higher rejects more, including some real faces.",
                    0.0, 1.0, 0.05),

                new ParameterDescriptor("nmsThreshold", "Overlap threshold", ParameterKind.Number,
                    "How much two detections may overlap before they are treated as one face.",
                    0.0, 1.0, 0.05),

                new ParameterDescriptor("minSize", "Smallest face", ParameterKind.Number,
                    "As a fraction of the frame. Raising it makes the detector ignore distant faces and run faster.",
                    0.01, 1.0, 0.01),

                new ParameterDescriptor("maxSize", "Largest face", ParameterKind.Number,
                    "As a fraction of the frame.",
                    0.05, 1.0, 0.05),

                new ParameterDescriptor("imageScale", "Detection scale", ParameterKind.Number,
                    "The frame is scaled by this before detection. Below 1 is faster and finds fewer small faces.",
                    0.1, 2.0, 0.1),

                new ParameterDescriptor("topK", "Candidates kept", ParameterKind.Integer,
                    "How many raw detections are considered before the overlap filter.",
                    1.0, 500.0, 10.0),

                new ParameterDescriptor("fileName", "Model file", ParameterKind.Text,
                    "The ONNX network, relative to the working directory.")
            }),

        new ModuleDescriptor("faceTracker", "Face tracker",
            "Carries each face from frame to frame and owns its identity. This is what makes a user a user rather than a detection.",
            new[]
            {
                new ParameterDescriptor("maxTracks", "Faces followed", ParameterKind.Integer,
                    "The upper limit on faces tracked at once. Each one costs a full pass of the shape model.",
                    1.0, 8.0, 1.0),

                new ParameterDescriptor("trackAwaySec", "Forget after", ParameterKind.Number,
                    "How long a face that has left the frame keeps its identity, so that coming back is not a new user.",
                    0.0, 120.0, 1.0),

                new ParameterDescriptor("trackOverlap", "Match overlap", ParameterKind.Number,
                    "How much a detection and a track have to overlap to be the same face.",
                    0.0, 1.0, 0.05),

                new ParameterDescriptor("templateScale", "Template size", ParameterKind.Number,
                    "The part of the face the tracker matches on, as a fraction of the rectangle.",
                    0.05, 1.0, 0.05)
            }),

        new ModuleDescriptor("shapeModel", "Shape model",
            "Fits the 68 landmarks with 3DDFA. The most expensive stage, and the one everything downstream is built on.",
            new[]
            {
                new ParameterDescriptor("smoothing", "Smooth the fit", ParameterKind.Boolean,
                    "Eases the landmarks between frames. Steadier, at the cost of a little lag on fast movement."),

                new ParameterDescriptor("parallelUsers", "Fit faces in parallel", ParameterKind.Boolean,
                    "Runs the fit for several faces at once. No effect while only one face is tracked."),

                new ParameterDescriptor("modelDir", "Model directory", ParameterKind.Text,
                    "Where the network lives, relative to the working directory.")
            }),

        new ModuleDescriptor("headPose", "Head pose",
            "Places the head in the camera's space: where it is, and which way it is facing.",
            new[]
            {
                new ParameterDescriptor("poseFromShapeModel", "Take the rotation from the fit", ParameterKind.Boolean,
                    "The shape model regresses a head rotation of its own, and it is the steadier of the two. "
                    + "Off, the rotation is solved from the landmarks against the average face - which is not "
                    + "your face, so the best fit of it wanders, and the head is seen to swing about while you sit still."),

                new ParameterDescriptor("estimateReprojection", "Check reprojection", ParameterKind.Boolean,
                    "Measures how well the solved pose explains the landmarks. Useful for diagnosis, extra work otherwise."),

                new ParameterDescriptor("faceBoxOffset", "Face box margin", ParameterKind.Number,
                    "How far the drawn box sits outside the face.",
                    0.0, 50.0, 1.0),

                new ParameterDescriptor("parallelUsers", "Solve faces in parallel", ParameterKind.Boolean,
                    "Runs the solver for several faces at once.")
            }),

        new ModuleDescriptor("shapeNorm", "Shape normalisation",
            "Aligns each shape to the canonical face, which is what makes two faces comparable.",
            new[]
            {
                new ParameterDescriptor("maxCount", "Iteration limit", ParameterKind.Integer,
                    "The most alignment steps taken before the result is accepted as it is.",
                    1.0, 10000.0, 100.0),

                new ParameterDescriptor("epsilon", "Convergence", ParameterKind.Text,
                    "How small a step has to be for the alignment to stop."),

                new ParameterDescriptor("parallelUsers", "Normalise faces in parallel", ParameterKind.Boolean,
                    "Runs the alignment for several faces at once.")
            }),

        new ModuleDescriptor("userHistory", "User history",
            "Remembers the users that have been seen and forgets the ones that are long gone.",
            new[]
            {
                new ParameterDescriptor("removeFreqMs", "Clean-up interval", ParameterKind.Integer,
                    "How often the record of departed users is swept.",
                    100.0, 60000.0, 500.0)
            }),

        new ModuleDescriptor("visualizer", "Visualizer",
            "The pipeline's own overlay, drawn into the frame with OpenCV. This application normally draws its own instead, on the GPU.",
            new[]
            {
                new ParameterDescriptor("glow", "Glow", ParameterKind.Boolean,
                    "A blurred halo behind the mesh."),

                new ParameterDescriptor("panel", "User panel", ParameterKind.Boolean,
                    "The per-user card drawn into the corner of the frame."),

                new ParameterDescriptor("poseBox", "Pose box", ParameterKind.Boolean,
                    "The projected box around the head.")
            })
    };
}
