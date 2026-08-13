# Introduction

This cross-platform face API is about achieving facial information driven development (e.g. gesture-based control) in mobile environment. The scheme is purely based on the user's face or more precisely on the following data:

- Face rectangle in the 2-D pixel-, and the face box in the 3-D camera coordinate system
- 68 pieces of facial feature points in the 2-D pixel-, and 3-D camera coordinate system
- 6DoF head pose (to the camera) in the 3-D camera coordinate system

All the information is determined on monocular images therefore there is no need for a special hardware or sensor during the calculations. If you are interested in the technical background then feel free to read in the following publications: [1], [2] and [3]

The system architecture is designed to reuse the image processing algorithms in case of multiple host platforms. The code of application logic (AL) is written in C++ and is shared between the particular platforms. Only a thin-layer of platform-specific code are used for transferring data from/to the user interface (UI). For example, this platform-specific code is a JNI bridge in case of the Android host platform.

A detailed description about the architecture can be read in this paper [2]:<br>
https://www.researchgate.net/publication/312411829_Face_recognition_on_mobile_platforms

You can view the results on the video below:<br>
[![Face API](http://img.youtube.com/vi/iS4eDf775GI/0.jpg)](https://www.youtube.com/watch?v=iS4eDf775GI "Face API")<br>
https://www.youtube.com/watch?v=iS4eDf775GI

# Prerequisites

All dependencies of the compilation in Windows (Visual Studio 2022) and Android can be downloaded (as pre-built libraries) from the following repository:<br>
https://github.com/bkornel/3rdparty<br>

The API is built against OpenCV 4.14.0, and the Windows console application also uses Poco 1.10.1.

The Windows desktop application, [Face Studio](#face-studio), additionally needs the .NET 9 SDK; it pulls the Windows App SDK from NuGet on the first restore and needs no other installation.

You should follow the directory structure below during the compilation:
```
[local_path_of_the_project]
|- 3rdparty
|-- opencv-4.14.0
|-- poco-1.10.1
|-- ...
|- face-api
|-- Applications
|-- FaceApi
|-- Testing
|-- ...
```

# Building the API and the Applications

## Windows

There are two solutions in [Applications/Windows](https://github.com/bkornel/face-api/tree/master/Applications/Windows), and they build side by side into the same [Bin](https://github.com/bkornel/face-api/tree/master/Applications/Windows/Bin) directory:

- `FaceApp.sln` builds the console application, which opens a capture, asks the API for the rendered frame and shows it in an OpenCV window. It is the shortest path from a camera to a result.
- `FaceStudio.sln` builds [Face Studio](#face-studio), the desktop application, along with the native engine it runs on.

Nothing else should be set up. A pre-build step ([`Deploy-Dependencies.ps1`](https://github.com/bkornel/face-api/blob/master/Applications/Windows/BuildEvents/Deploy-Dependencies.ps1)) copies the OpenCV and Poco binaries and the model files next to the executables, and seeds each application's own working directory from its own project, so either can be started right after building. A missing dependency fails the build instead of producing an executable that cannot start.

The models are shared, the settings are not. `Bin/<configuration>/faceapp` and `Bin/<configuration>/studio` hold one `settings.json` each, because the two applications configure the graph differently. A deployed copy is only overwritten when the one in the project is newer, so a setting changed by hand or by the settings editor survives the next build.

## Android

The Android Studio project can be found in [Applications/Android](https://github.com/bkornel/face-api/tree/master/Applications/Android). The following packages must be installed via SDK manager

### SDK platforms
- Min SDK version: API 23 (Android 6.0)
- Target SDK version: API 28 (Android 9.0)

### SDK Tools
- LLDB 3.x or newer
- CMake 3.6.x or newer
- NDK 20.x or newer

The C++ part (image processing algorithms) is set up as a CMake external native build in the Android Studio project, thus it is built automatically when you make the project.

# Modules

The whole module graph can be created from the settings file (defined in [`settings.json`](https://github.com/bkornel/face-api/blob/master/Applications/Windows/FaceApp/Configurations/settings.json) by default). Modules can interact and exchange information whith each others via ports. Every module must have one output port and can have any input ports (zero or more).

Ports can be defined by implementing the `fw::Port` interface. For example the [`FaceTracker`](https://github.com/bkornel/face-api/blob/master/FaceApi/Modules/FaceTracker/FaceTracker.h) class:

```
class FaceTracker :
  public fw::Module,
  public fw::Port<std::shared_ptr<FaceTrackMessage>(std::shared_ptr<ImageMessage>, std::shared_ptr<RoiMessage>)>
{
  ...
  std::shared_ptr<FaceTrackMessage> Main(std::shared_ptr<ImageMessage> iImage, std::shared_ptr<RoiMessage> iDetections) override;
  ...
};
```

This class has the `FaceTrackMessage` output and the `ImageMessage` and `RoiMessage` inputs. The `Main` member function of the class must defined according to the template arguments of `fw::Port`.

A module that declares no input port at all is a source: it has nothing to wait for, so it is driven by `Trigger()` instead of by a predecessor. This is how [`FirstModule`](https://github.com/bkornel/face-api/blob/master/FaceApi/Modules/FirstModule/FirstModule.h) starts every frame. Input ports are optional as well: a port that is left out of the settings file is not counted as a dependency, and its argument reaches `Main` default-constructed. A module with declared inputs of which none is connected is rejected, because nothing would ever make it run.

## Module Graph

The module graph can be defined in the settings file, one per application - the console application's is [here](https://github.com/bkornel/face-api/blob/master/Applications/Windows/FaceApp/Configurations/settings.json). For the [`FaceTracker`](https://github.com/bkornel/face-api/blob/master/FaceApi/Modules/FaceTracker/FaceTracker.h) module it is:

```
"faceTracker": {
  "port": [ "imageQueue:1", "faceDetection:2" ],
  ...
}
```

Where the [`ImageQueue`](https://github.com/bkornel/face-api/blob/master/FaceApi/Modules/ImageQueue/ImageQueue.h) module returns with an `ImageMessage` and transfers the information to the first input port of [`FaceTracker`](https://github.com/bkornel/face-api/blob/master/FaceApi/Modules/FaceTracker/FaceTracker.h) and so does the [`FaceDetection`](https://github.com/bkornel/face-api/blob/master/FaceApi/Modules/FaceDetection/FaceDetection.h) with `RoiMessage`.

The pipeline itself is a chain of value messages: the tracker owns the identity of every face and publishes plain track records; `shapeModel`, `headPose` and `shapeNorm` enrich a per-face result record without touching any shared state; `userManager` at the end composes the records into immutable users. No module ever writes into an object another module holds.

## Getting the Results

There are two ways to read what the API determined. `FaceApi::GetResultImage` returns the frame the [`Visualizer`](https://github.com/bkornel/face-api/blob/master/FaceApi/Modules/Visualizer/Visualizer.h) drew the overlay on, which is the simplest way to display something and what the console application uses. `FaceApi::GetResults` returns the same information as data instead, and all of it: the face rectangle, the feature points in 2-D and 3-D, the face box, the head pose, whether the identity was just detected or is being tracked and for how long, the reference-aligned shapes, and the expression measures. It reports everything the pipeline worked out precisely so that a host does not derive any of it a second time — a host watching identities across frames to guess which one is new would be doing it worse, and once per platform. This is the path [Face Studio](#face-studio) takes. For hosts that cannot take C++ structures across their language boundary, `face::result_buffer` packs the results into a flat float buffer; the Android application reads that from its JNI bridge.

Drawing on the host side is the cheaper path, because the frame is neither composited nor copied back, which matters on a mobile device. It is switched on by leaving the `visualizer` module out of the settings file, in which case `lastModule` takes the frame from the queue directly:

```
"lastModule": {
  "port": [ "imageQueue:1", "userManager:2" ]
}
```

The second port carries the users and is the one `GetResults` reports. It is optional, so a graph that only needs the rendered frame can leave it out.

## Adding a New Module

There is one place to extend: `ModuleFactory::Create(...)` in [`ModuleFactory.cpp`](https://github.com/bkornel/face-api/blob/master/FaceApi/Modules/ModuleFactory.cpp), which maps the name used in the settings file to the class that implements it. Look for the `// REMARK: Insert new modules here` comment; it is one line.

Connecting the module is automatic. A module declares its ports by deriving from `fw::Port`, which implements [`IPortConnector`](https://github.com/bkornel/face-api/blob/master/FaceApi/Framework/Graph/IPortConnector.h), and [`ModuleConnector`](https://github.com/bkornel/face-api/blob/master/FaceApi/Modules/ModuleConnector.cpp) wires the graph through that interface alone, so it never has to know which module it is holding. An output travels between the two as an `fw::IFuture` and the input side casts it back to the type its own port number expects, which means a port wired to a module publishing something else is reported as exactly that, naming the two modules and the port.

Beside these, the `Main` of a new module should start with a `DrainCommands()` call. Commands, for example that the image size has changed, are published on a message bus by the thread that raises them, and `DrainCommands` applies them on the thread of the module graph. This is what keeps a module's own state touched from one thread only.

# Face Studio

The Windows desktop application, in [Applications/Windows/FaceStudio](https://github.com/bkornel/face-api/tree/master/Applications/Windows/FaceStudio). It opens a camera or a video file, runs it through the API and shows what came out: the frame with the overlay on it, an inspector for every tracked face, the pipeline's own statistics over time, and and an artificial head that mimics the user.

It is the second way of using the API rather than a replacement for the first. The console application asks for a rendered frame; this one asks for the results and draws them itself, which is the path a host with its own canvas would take.

## How it is put together

Three layers, and the split between them is the point:

| | |
| --- | --- |
| `FaceApi` | Platform-independent, native C++. The pipeline, and everything that is true of a face regardless of who is showing it: the expression measures, the normalised shapes, which landmarks join into which stroke, where the pose axes point, and the avatar's geometry. |
| `FaceEngine` | A Windows DLL with a flat C interface. Capture, and both renderers: Direct2D for the frame and the overlay, Direct3D 11 for the avatar. |
| `FaceStudio` | A WinUI 3 application in C#. The window, the panels and the settings editor. |

There was briefly a fourth, `FaceView`, holding the presentation code that is not platform-specific. It has been folded into `FaceApi`, because the line it drew did not survive contact: it was platform-independent C++ that depended on `FaceApi`, compiled into every host that already compiled `FaceApi`, and `Visualizer` — presentation by any measure — had always lived inside `FaceApi` anyway. Keeping it apart bought a second project file and a second `CMakeLists.txt` to keep in step, and nothing else.

Folding it in also collapsed three descriptions of the same thing into one. The stroke groups of the 68-point layout, the depth fading and the pose-gizmo directions each existed twice — once in `Visualizer`, once in the presentation library — and now live in [`FaceModel`](https://github.com/bkornel/face-api/blob/master/FaceApi/Model/FaceModel.h), [`Geometry`](https://github.com/bkornel/face-api/blob/master/FaceApi/Framework/Imaging/Geometry.h) and [`PoseGeometry`](https://github.com/bkornel/face-api/blob/master/FaceApi/Model/PoseGeometry.h), read by both.

No pixel crosses the boundary between the second and the third. The engine hands out its swap chains, the application binds them to its `SwapChainPanel`s, and the frames are composited by the system. What does cross is one small struct per UI tick - twenty a second, not thirty-odd - carrying the numbers the panels display. That is what keeps the cost of the interop independent of the frame size and of the frame rate.

What that leaves in the platform layer is only the code that touches a screen. Most of what this application does is not Windows work, and when the Android application is modernised it can read the same measures and build the same avatar from the API it already compiles — it will only have to write its own renderer.

## The avatar

The head is an authored model, [ICT-FaceKit's generic neutral head](https://github.com/bkornel/face-api/tree/master/Testing/configurations/shapemodel/head) — MIT licensed, 26719 quads, with ears, a neck, a mouth socket and two eyeballs. It is loaded rather than generated, and that is the whole point: sixty-eight landmarks cannot describe a head, so nothing built out of them alone has an ear or an eyelid that folds. What they *can* do is move one that already has them.

The two meet exactly rather than approximately. ICT-FaceKit publishes the vertex indices of the 68 Multi-PIE landmarks on its own topology, which is the same layout the pipeline speaks, so [`HeadMesh`](https://github.com/bkornel/face-api/blob/master/FaceApi/Model/HeadMesh.h) binds one to the other by table lookup. Measured across every pair of landmarks, the model's face and the pipeline's canonical face agree to a mean of 0.08 inter-ocular spans — the residual being that they are two different faces, which is exactly the difference the avatar exists to show.

Each frame the surface is displaced by how far the tracked face departs from the average face, so the head takes on the user's own proportions and does what they are doing. Every vertex follows a weighted average of the landmarks near it, worked out once at load so that a frame is a matrix multiply.

The weights are normalised, and the bound that gives is the point: a vertex can never move further than the furthest landmark did, so a strong expression cannot spike or fold the surface. Solving instead for a field that passes exactly through every landmark is more faithful for a small movement and comes apart under a large one — the weights are large and alternate in sign, and a strong expression multiplies them into creases. Nothing is lost to the bound, because a landmark's weight grows without limit as a vertex approaches it: a vertex on a landmark follows it exactly.

Distance is measured **along the surface**, not through space, and that is what lets a mouth open at all. Through space the upper and lower lip pass within a hundredth of an eye span of one another, and no field that is smooth in space can move one without moving the other — the lips can only ever stretch. Along the surface they are 0.59 apart, because the way between them goes round the corner of the mouth. The same holds for the eyelids.

Each landmark then reaches as far as its own neighbours are distant, and the two constraints that pin the multiplier can only both be met on the surface: a lip's reach must stay under 0.59 or the lips move as one, and a brow's must exceed 0.64 or a raised brow leaves the forehead behind. Beyond that reach the surface does not move at all, which is what keeps the back of the head still while the face works.

What drives it is `FaceResult::normShape3D` — the tracked shape with its position, scale and rotation already removed by the pipeline's `shapeNorm` module. The mesh turns it the last few degrees onto the canonical face, because the pipeline aligns onto the mean its own alignment converged on and the displacement has to be measured against the model exactly.

That shape now comes from the fitter rather than from the pose module, and the difference is the whole of whether the avatar moves. The morphable model reconstructs the 68 landmarks in three dimensions and projects them to get the 2-D ones; the 3-D shape was being discarded, and the only three-dimensional shape the pipeline reported was the *canonical* model moved to where the head is. That one is the same shape on every frame of every face, so anything driven by it sits perfectly still. `ShapeDescriptor::shape3D` carries the fitted shape now, and `shapeNorm` normalises that.

The shading is stylised where the geometry is not: the light falls in three steps rather than a ramp and a contour is drawn where the surface turns away from the viewer, which keeps a 26000-quad head legible at the size a panel gives it.

## Configuration

Face Studio has its own working directory, `Bin/<configuration>/studio`, and its own `settings.json` in it, seeded from [Applications/Windows/FaceStudio/Configurations](https://github.com/bkornel/face-api/tree/master/Applications/Windows/FaceStudio/Configurations). It differs from the console application's in one way that matters: the `visualizer` module is left out of the graph, because the overlay is drawn on the GPU here. Both point their model directories back at the shared `configurations` folder rather than duplicating it.

The Pipeline page is a full editor for that file. Every parameter it knows about carries a range and an explanation - the file itself is untyped strings, and a settings editor that only offered text boxes would be a worse way to edit it than a text editor. Saving writes the file and rebuilds the graph without restarting the application, with the source still running.

The first card on that page is the one that decides which of the two ways of using the API is in effect: it moves the `visualizer` module in and out of the graph and rewires `lastModule` to match. With the pipeline drawing, the application shows the frame as it is and says so.

# References

> [1]	K. Bertok and A. Fazekas, "Facial gesture recognition and its applications," In Gesture recognition: performance, applications and features, Nova Science Publishers, New York, pp. 1-30, 2018.<br>
> [2]	K. Bertok and A. Fazekas, "Face recognition on mobile platforms," In Proceedings of the 7th IEEE International Conference on Cognitive Infocommunications: CogInfoCom 2016, pp. 37-43, 2016.<br>
> [3]	K. Bertok and A. Fazekas, "Recognizing complex head movements," In Australian Journal of Intelligent Information Processing Systems, vol. 14, pp. 3-17, 2016.
