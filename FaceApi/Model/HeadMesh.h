#pragma once

#include "Framework/Imaging/Geometry.h"

#include <opencv2/core/core.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace face
{
  /// @brief An artificial head that follows a tracked face.
  ///
  /// The head itself is an authored model - ICT-FaceKit's generic neutral head, MIT licensed
  /// - loaded from disk rather than generated. Sixty-eight points cannot describe a head, so
  /// nothing built out of them alone has ears, a neck or an eyelid that folds; what they can
  /// do is *move* one that already has them, which is what this class is for.
  ///
  /// Each vertex is bound once to the landmarks near it with a Gaussian falloff. Every frame
  /// the surface is then displaced by how far the tracked face departs from the average face,
  /// so the head takes on the user's proportions and does what they are doing: opens its
  /// mouth, raises its brows, turns its jaw. It costs a few multiplications per vertex, which
  /// is why it belongs in the API rather than in a shader - an Android renderer wants the
  /// same buffer.
  ///
  /// Everything is in the frame the pipeline's normalised shapes come in: the centroid of the
  /// face at the origin, x to the user's left on screen, y downwards, z away from the viewer,
  /// one unit between the outer eye corners. A renderer applies its own view transform.
  class HeadMesh
  {
  public:
    /// @brief What a vertex belongs to, so that one draw call can paint skin, eyes and the
    /// inside of the mouth differently.
    enum class Material : int
    {
      Skin = 0,
      EyeWhite = 1,
      Iris = 2,
      Pupil = 3,
      MouthCavity = 4
    };

    struct Vertex
    {
      cv::Vec3f position;
      cv::Vec3f normal;

      /// @brief One of Material, as a float because that is how it reaches a shader
      float material;

      /// @brief Baked occlusion in [0, 1]. Measured from how concave the surface is around
      /// the vertex, which is what makes the eye sockets and the nostrils read as recesses.
      float occlusion;
    };

    struct BuildParams
    {
      /// @brief The furthest any landmark's influence may reach, in inter-ocular spans.
      ///
      /// Each landmark gets its own reach, scaled to how far its neighbours are: the eight
      /// inner lip points sit a twentieth of a span apart and need a short one, while the
      /// brows are the only thing anywhere near the forehead and need a long one. A single
      /// reach for all of them cannot do both - long enough for the forehead to follow a
      /// raised brow is long enough for the lips to average each other away. This is the
      /// ceiling on that per-landmark value, and what keeps the back of the head still.
      double reach = 0.90;
    };

    /// @brief What the head should be doing this frame.
    struct DriveInput
    {
      /// @brief The tracked face as the pipeline normalised it - FaceResult::normShape3D -
      /// or null to ease back to the neutral head
      const fw::VectorPt3D* normalisedShape = nullptr;

      /// @brief How much of the face's departure from the average to apply. 1 puts the
      /// user's own proportions and expression on the head; above that caricatures; 0 leaves
      /// the authored head untouched.
      double expression = 1.2;

      /// @brief How much of the previous frame to keep, in [0, 1). Fitting noise is what
      /// this is for: at 0 the head twitches with every frame the fit lands differently.
      double smoothing = 0.55;

      /// @brief Whether this is a shape the caller has not passed before.
      ///
      /// A host normally renders faster than the pipeline produces frames, so Drive() is
      /// called several times per shape. Smoothing every call would make the amount of it
      /// depend on the frame rate, which is not something the caller asked for.
      bool shapeIsNew = true;
    };

    HeadMesh() = default;

    HeadMesh(const HeadMesh& iOther) = delete;

    HeadMesh& operator=(const HeadMesh& iOther) = delete;

    /// @brief Loads the head and binds it to the landmarks. Must be called before Drive().
    /// @param iPath The OBJ, normally shapemodel/head/ict_neutral_head.obj
    /// @return false when the file cannot be read or is not the topology this expects
    bool Build(const std::string& iPath, const BuildParams& iParams = BuildParams());

    inline bool IsBuilt() const
    {
      return !mIndices.empty();
    }

    /// @brief Moves the surface to what the face is doing, and rebuilds the normals.
    void Drive(const DriveInput& iInput);

    inline const std::vector<Vertex>& GetVertices() const
    {
      return mVertices;
    }

    inline const std::vector<uint32_t>& GetIndices() const
    {
      return mIndices;
    }

    /// @brief The head itself, without the eyes: the first part of GetIndices()
    inline uint32_t GetSurfaceIndexCount() const
    {
      return mSurfaceIndexCount;
    }

    /// @brief The eyes, which a host may leave out: the rest of GetIndices()
    inline uint32_t GetEyeIndexOffset() const
    {
      return mSurfaceIndexCount;
    }

    inline uint32_t GetEyeIndexCount() const
    {
      return static_cast<uint32_t>(mIndices.size()) - mSurfaceIndexCount;
    }

    /// @brief The surface as a line list, for hosts that offer a wireframe view
    inline const std::vector<uint32_t>& GetEdgeIndices() const
    {
      return mEdgeIndices;
    }

    /// @brief Where the landmarks ended up after the last Drive(), in mesh space
    inline const std::vector<cv::Vec3f>& GetLandmarkPositions() const
    {
      return mDrivenLandmarks;
    }

    /// @brief Which vertex of the mesh each of the 68 landmarks is.
    ///
    /// Worth having rather than searching for the nearest vertex to a landmark's position:
    /// the two lips pass within a hundredth of an eye span of one another, so a search finds
    /// whichever of them happens to be closer and can easily answer with the wrong one.
    inline const std::vector<int>& GetLandmarkVertices() const
    {
      return mLandmarkVertices;
    }

    /// @brief Centre of the head, for a host placing a camera on it
    inline const cv::Vec3f& GetCentre() const
    {
      return mCentre;
    }

    /// @brief Distance from the centre to the furthest vertex of the neutral head
    inline float GetRadius() const
    {
      return mRadius;
    }

    /// @brief Puts a shape in the frame this class works in: the centroid of its landmarks
    /// at the origin, one unit between the outer eye corners.
    /// @return false when the shape is not the 68-point layout, leaving oShape untouched
    static bool ToMeshSpace(const fw::VectorPt3D& iShape, std::vector<cv::Vec3f>& oShape);

  private:
    /// @brief One part of the model, as the OBJ groups it
    enum class Part
    {
      Skin,
      MouthSocket,
      Eyeball
    };

    bool Load(const std::string& iPath);

    /// @brief Drops the vertices no kept triangle references, remapping everything that
    /// addresses them. The asset keeps them so that the published landmark indices stay
    /// valid; once those have been read they are only weight.
    void Compact();

    /// @brief Moves the loaded head into mesh space, using its own landmarks as the frame
    bool Normalise();

    /// @brief Builds the vertex adjacency the geodesic distances walk over
    void BuildAdjacency();

    /// @brief Distance from one landmark to every vertex, measured along the surface rather
    /// than through space, and stopped once it passes iCutoff.
    ///
    /// Through space, the upper and lower lip are a hair apart when the mouth is shut, and
    /// any smooth field of space gives them the same displacement - so they can never part.
    /// Along the surface they are as far apart as the way round the mouth corner, which is
    /// what lets a mouth open and an eye close.
    void GeodesicFrom(uint32_t iVertex, double iCutoff, std::vector<float>& oDistance) const;

    /// @brief Works out once how each vertex follows the landmarks, so that every frame is
    /// a matrix multiply.
    ///
    /// Every row sums to one and holds nothing negative, which makes each vertex's
    /// displacement a weighted average of the landmarks' - and therefore never larger than
    /// the largest of them. That bound is the point. Solving instead for a field that passes
    /// exactly through every landmark gives weights that are large and alternate in sign,
    /// and while that is faithful for a small movement, a strong expression multiplies those
    /// weights into spikes and folds and the head comes apart.
    ///
    /// Exactness is not given up for it: the weight of a landmark grows without bound as a
    /// vertex approaches it, so a vertex sitting on one follows it exactly and its
    /// neighbours nearly so.
    bool SolveDeformation(const BuildParams& iParams);

    void AssignMaterials();

    void BakeOcclusion();

    /// @brief Turns a shape onto the neutral face, so that the difference between the two is
    /// what this face is and does rather than where it was looking
    void AlignToNeutral(std::vector<cv::Vec3f>& ioShape) const;

    void ApplyDisplacement(double iExpression);

    /// @brief Carries the eyeballs with the eye region without deforming them: an eyeball is
    /// a sphere and has to stay one, however much the lids around it move
    void PlaceEyeballs();

    void RecomputeNormals();

    /// @brief Vertex indices of the 68 landmarks on the loaded head
    std::vector<int> mLandmarkVertices;

    /// @brief The head's own landmarks, in mesh space. The bindings are anchored here and
    /// this is the pose the head returns to.
    std::vector<cv::Vec3f> mBaseLandmarks;

    /// @brief The average face the tracked shape is measured against, in mesh space. It is
    /// the canonical model of FaceModel, which is the shape the pipeline normalises onto.
    std::vector<cv::Vec3f> mNeutralLandmarks;

    /// @brief The shape currently driving the head, smoothed towards the measured one
    std::vector<cv::Vec3f> mDrivenLandmarks;

    /// @brief Scratch, so that Drive() allocates nothing
    std::vector<cv::Vec3f> mIncomingLandmarks;
    std::vector<cv::Vec3f> mLandmarkDeltas;

    /// @brief The head as it was loaded, before any landmark moved
    std::vector<cv::Vec3f> mRestPositions;

    /// @brief One row of 68 weights per vertex, laid out flat: the displacement of vertex v
    /// is the sum of mWeights[v * 68 + j] times the displacement of landmark j
    std::vector<float> mWeights;

    /// @brief Vertices no landmark reaches, so they can be skipped rather than multiplied
    /// by a row of zeros
    std::vector<bool> mIsDeformed;

    /// @brief The neighbours of every vertex, as a compressed row: mAdjacency between
    /// mAdjacencyStart[v] and mAdjacencyStart[v + 1]
    std::vector<uint32_t> mAdjacencyStart;
    std::vector<uint32_t> mAdjacency;

    std::vector<Part> mParts;
    std::vector<Vertex> mVertices;
    std::vector<uint32_t> mIndices;
    std::vector<uint32_t> mEdgeIndices;

    /// @brief The vertex range of each eyeball, so they can be moved rigidly
    std::vector<std::pair<uint32_t, uint32_t>> mEyeballRanges;

    uint32_t mSurfaceIndexCount = 0U;

    cv::Vec3f mCentre{ 0.0F, 0.0F, 0.0F };
    float mRadius = 1.0F;

    bool mHasDrivenShape = false;
  };
}
