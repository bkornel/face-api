#include "Model/HeadMesh.h"

#include "Model/FaceModel.h"
#include "Model/ShapeMetrics.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <queue>
#include <sstream>
#include <unordered_map>

namespace face
{
  namespace
  {
    /// @brief Vertex indices of the 68 Multi-PIE landmarks on ICT-FaceKit's head, in the
    /// order the layout numbers them. Published in the ICT-FaceKit README, next to the model
    /// itself; the licence and provenance are in the asset's own header.
    ///
    /// This is what makes an authored head usable at all: without it the landmarks would have
    /// to be guessed by proximity, and a guess at the eye corners is a squint.
    constexpr int sLandmarkVertices[] = {
      1225, 1888, 1052, 367, 1719, 1722, 2199, 1447, 966, 3661, 4390, 3927, 3924, 2608,
      3272, 4088, 3443, 268, 493, 1914, 2044, 1401, 3615, 4240, 4114, 2734, 2509, 978,
      4527, 4942, 4857, 1140, 2075, 1147, 4269, 3360, 1507, 1542, 1537, 1528, 1518, 1511,
      3742, 3751, 3756, 3721, 3725, 3732, 5708, 5695, 2081, 0, 4275, 6200, 6213, 6346,
      6461, 5518, 5957, 5841, 5702, 5711, 5533, 6216, 6207, 6470, 5517, 5966
    };

    constexpr std::size_t sLandmarkCount = sizeof(sLandmarkVertices) / sizeof(sLandmarkVertices[0]);

    /// @brief How far into the forward-facing cap of an eyeball the iris and the pupil reach,
    /// as the cosine of the angle from straight ahead
    constexpr float sIrisCosine = 0.86F;
    constexpr float sPupilCosine = 0.965F;

    inline float Dot(const cv::Vec3f& iA, const cv::Vec3f& iB)
    {
      return iA[0] * iB[0] + iA[1] * iB[1] + iA[2] * iB[2];
    }

    inline float Length(const cv::Vec3f& iV)
    {
      return std::sqrt(Dot(iV, iV));
    }

    inline cv::Vec3f Normalised(const cv::Vec3f& iV)
    {
      const float length = Length(iV);
      return length > 1e-8F ? iV / length : cv::Vec3f(0.0F, 0.0F, -1.0F);
    }

    inline cv::Vec3f Cross(const cv::Vec3f& iA, const cv::Vec3f& iB)
    {
      return {
        iA[1] * iB[2] - iA[2] * iB[1],
        iA[2] * iB[0] - iA[0] * iB[2],
        iA[0] * iB[1] - iA[1] * iB[0]
      };
    }

    inline int Index(Landmark iLandmark)
    {
      return index_of(iLandmark);
    }

    /// @brief How much a landmark is heard at a given distance, before the weights of one
    /// vertex are normalised against one another.
    ///
    /// It grows without bound as the distance falls to nothing, so a vertex on a landmark
    /// hears that landmark and nothing else - which is what keeps the movement faithful once
    /// the weights are normalised. And it falls to exactly nothing at iSupport, so the face
    /// can work without the back of the head hearing any of it.
    inline double Weight(double iDistance, double iSupport)
    {
      if (iDistance >= iSupport) return 0.0;
      if (iDistance <= 1e-6) return 1e12;

      const double taper = (iSupport - iDistance) / (iSupport * iDistance);

      return taper * taper;
    }
  }

  bool HeadMesh::ToMeshSpace(const fw::VectorPt3D& iShape, std::vector<cv::Vec3f>& oShape)
  {
    const double span = interocular_distance(iShape);
    if (span < 1e-9) return false;

    cv::Point3d centroid(0.0, 0.0, 0.0);
    for (const auto& point : iShape) centroid += point;

    centroid /= static_cast<double>(iShape.size());

    oShape.resize(iShape.size());

    for (std::size_t i = 0U; i < iShape.size(); ++i)
    {
      const cv::Point3d moved = (iShape[i] - centroid) / span;

      oShape[i] = { static_cast<float>(moved.x), static_cast<float>(moved.y), static_cast<float>(moved.z) };
    }

    return true;
  }

  bool HeadMesh::Build(const std::string& iPath, const BuildParams& iParams)
  {
    mVertices.clear();
    mIndices.clear();
    mEdgeIndices.clear();
    mEyeballRanges.clear();
    mHasDrivenShape = false;

    if (!Load(iPath)) return false;
    if (!Normalise()) return false;

    // The average face the tracked shape is measured against. It is the same canonical model
    // the pipeline aligns its normalised shapes onto, so the difference between the two is
    // this face's own departure from the average and nothing else.
    if (!ToMeshSpace(FaceModel::GetInstance().GetShape3D(), mNeutralLandmarks)) return false;

    mDrivenLandmarks = mNeutralLandmarks;

    AssignMaterials();

    if (!SolveDeformation(iParams)) return false;

    BakeOcclusion();
    RecomputeNormals();

    // The middle of the head rather than of the face, so that orbiting circles the head
    cv::Vec3f low = mRestPositions.front();
    cv::Vec3f high = mRestPositions.front();

    for (const auto& position : mRestPositions)
    {
      for (int a = 0; a < 3; ++a)
      {
        low[a] = (std::min)(low[a], position[a]);
        high[a] = (std::max)(high[a], position[a]);
      }
    }

    mCentre = (low + high) * 0.5F;

    mRadius = 0.5F;
    for (const auto& position : mRestPositions)
    {
      mRadius = (std::max)(mRadius, Length(position - mCentre));
    }

    return true;
  }

  bool HeadMesh::Load(const std::string& iPath)
  {
    std::ifstream file(iPath);
    if (!file.is_open()) return false;

    std::vector<cv::Vec3f> positions;
    positions.reserve(32768U);

    // The eyes are drawn separately from the head, so their triangles are collected apart
    // and appended after it - which is what lets a host leave them out with an offset
    std::vector<uint32_t> surfaceIndices;
    std::vector<uint32_t> eyeIndices;
    std::vector<Part> partOfVertex;

    Part current = Part::Skin;
    bool inEyeball = false;
    uint32_t eyeballFirst = 0U;
    uint32_t eyeballLast = 0U;

    std::string line;
    std::string keyword;

    while (std::getline(file, line))
    {
      if (line.empty() || line[0] == '#') continue;

      std::istringstream stream(line);
      stream >> keyword;

      if (keyword == "v")
      {
        float x = 0.0F;
        float y = 0.0F;
        float z = 0.0F;

        stream >> x >> y >> z;

        // The model is authored with y up and z towards the viewer; the pipeline works in
        // image orientation, y down and z away. Two axes flip, once, here.
        positions.emplace_back(cv::Vec3f{ x, -y, -z });
        continue;
      }

      if (keyword == "g")
      {
        std::string name;
        stream >> name;

        if (inEyeball && eyeballLast >= eyeballFirst)
        {
          mEyeballRanges.emplace_back(eyeballFirst, eyeballLast);
        }

        inEyeball = name.rfind("Eyeball", 0U) == 0U;

        current = inEyeball ? Part::Eyeball
                            : (name == "MouthSocket" ? Part::MouthSocket : Part::Skin);

        eyeballFirst = static_cast<uint32_t>(positions.size());
        eyeballLast = 0U;
        continue;
      }

      if (keyword != "f") continue;

      // The model is quads with a few triangles; both become triangles here
      std::vector<uint32_t> corners;
      std::string token;

      while (stream >> token)
      {
        const std::size_t slash = token.find('/');
        if (slash != std::string::npos) token = token.substr(0U, slash);

        const long value = std::strtol(token.c_str(), nullptr, 10);
        if (value <= 0) continue;

        corners.emplace_back(static_cast<uint32_t>(value - 1));
      }

      if (corners.size() < 3U) continue;

      partOfVertex.resize(positions.size(), Part::Skin);

      for (uint32_t corner : corners)
      {
        if (corner >= positions.size()) return false;

        partOfVertex[corner] = current;

        if (inEyeball)
        {
          eyeballFirst = (std::min)(eyeballFirst, corner);
          eyeballLast = (std::max)(eyeballLast, corner);
        }
      }

      std::vector<uint32_t>& target = inEyeball ? eyeIndices : surfaceIndices;

      for (std::size_t i = 1U; i + 1U < corners.size(); ++i)
      {
        target.insert(target.end(), { corners[0], corners[i], corners[i + 1U] });
      }

      // One line per edge of the polygon, which is the topology the model was authored with
      for (std::size_t i = 0U; i < corners.size(); ++i)
      {
        mEdgeIndices.insert(mEdgeIndices.end(), { corners[i], corners[(i + 1U) % corners.size()] });
      }
    }

    if (inEyeball && eyeballLast >= eyeballFirst) mEyeballRanges.emplace_back(eyeballFirst, eyeballLast);

    if (positions.size() <= static_cast<std::size_t>(*std::max_element(std::begin(sLandmarkVertices), std::end(sLandmarkVertices))))
      return false;

    if (surfaceIndices.empty()) return false;

    partOfVertex.resize(positions.size(), Part::Skin);

    mParts = std::move(partOfVertex);
    mRestPositions = std::move(positions);

    mSurfaceIndexCount = static_cast<uint32_t>(surfaceIndices.size());

    mIndices = std::move(surfaceIndices);
    mIndices.insert(mIndices.end(), eyeIndices.begin(), eyeIndices.end());

    mLandmarkVertices.assign(std::begin(sLandmarkVertices), std::end(sLandmarkVertices));

    Compact();

    return true;
  }

  void HeadMesh::Compact()
  {
    // The parts that were trimmed out of the asset left their vertices behind, so that the
    // published landmark indices would keep addressing the right ones. Now that those
    // indices have been read, the vertices nothing draws are dead weight: they would be
    // deformed, uploaded and framed against every frame without ever being seen.
    std::vector<int> remap(mRestPositions.size(), -1);

    for (uint32_t index : mIndices)
    {
      remap[index] = 0;
    }

    // A landmark must survive even if no kept triangle happens to touch it
    for (int landmark : mLandmarkVertices)
    {
      remap[static_cast<std::size_t>(landmark)] = 0;
    }

    std::vector<cv::Vec3f> positions;
    std::vector<Part> parts;

    positions.reserve(mRestPositions.size());
    parts.reserve(mParts.size());

    for (std::size_t i = 0U; i < mRestPositions.size(); ++i)
    {
      if (remap[i] < 0) continue;

      remap[i] = static_cast<int>(positions.size());

      positions.emplace_back(mRestPositions[i]);
      parts.emplace_back(mParts[i]);
    }

    for (auto& index : mIndices) index = static_cast<uint32_t>(remap[index]);

    // An edge of a dropped face may point at a vertex that did not survive
    std::vector<uint32_t> edges;
    edges.reserve(mEdgeIndices.size());

    for (std::size_t i = 0U; i + 1U < mEdgeIndices.size(); i += 2U)
    {
      const int a = remap[mEdgeIndices[i]];
      const int b = remap[mEdgeIndices[i + 1U]];

      if (a < 0 || b < 0) continue;

      edges.insert(edges.end(), { static_cast<uint32_t>(a), static_cast<uint32_t>(b) });
    }

    for (auto& landmark : mLandmarkVertices) landmark = remap[static_cast<std::size_t>(landmark)];

    for (auto& range : mEyeballRanges)
    {
      // The eyeball ranges were contiguous before and stay contiguous after, because the
      // compaction preserves order
      range.first = static_cast<uint32_t>(remap[range.first]);
      range.second = static_cast<uint32_t>(remap[range.second]);
    }

    mRestPositions = std::move(positions);
    mParts = std::move(parts);
    mEdgeIndices = std::move(edges);
  }

  bool HeadMesh::Normalise()
  {
    // The head is put into mesh space by its own landmarks, so that it lands where a tracked
    // shape lands: same origin, same scale, whatever the model was authored in
    fw::VectorPt3D landmarks(sLandmarkCount);

    for (std::size_t i = 0U; i < sLandmarkCount; ++i)
    {
      const cv::Vec3f& v = mRestPositions[static_cast<std::size_t>(mLandmarkVertices[i])];
      landmarks[i] = { v[0], v[1], v[2] };
    }

    const double span = interocular_distance(landmarks);
    if (span < 1e-9) return false;

    cv::Point3d centroid(0.0, 0.0, 0.0);
    for (const auto& point : landmarks) centroid += point;

    centroid /= static_cast<double>(landmarks.size());

    const cv::Vec3f origin(static_cast<float>(centroid.x), static_cast<float>(centroid.y), static_cast<float>(centroid.z));
    const float scale = 1.0F / static_cast<float>(span);

    for (auto& position : mRestPositions) position = (position - origin) * scale;

    mBaseLandmarks.resize(sLandmarkCount);

    for (std::size_t i = 0U; i < sLandmarkCount; ++i)
    {
      mBaseLandmarks[i] = mRestPositions[static_cast<std::size_t>(mLandmarkVertices[i])];
    }

    mVertices.resize(mRestPositions.size());

    for (std::size_t i = 0U; i < mRestPositions.size(); ++i)
    {
      mVertices[i].position = mRestPositions[i];
      mVertices[i].normal = { 0.0F, 0.0F, -1.0F };
      mVertices[i].material = static_cast<float>(Material::Skin);
      mVertices[i].occlusion = 1.0F;
    }

    return true;
  }

  void HeadMesh::AssignMaterials()
  {
    for (std::size_t i = 0U; i < mVertices.size(); ++i)
    {
      mVertices[i].material = mParts[i] == Part::MouthSocket
                                ? static_cast<float>(Material::MouthCavity)
                                : static_cast<float>(Material::Skin);
    }

    // An eyeball is a sphere, so the iris and the pupil are the cap of it that faces the way
    // the head faces. There is no gaze in the results, so the eyes look where the head looks.
    for (const auto& range : mEyeballRanges)
    {
      cv::Vec3f centre(0.0F, 0.0F, 0.0F);
      std::size_t count = 0U;

      for (uint32_t v = range.first; v <= range.second; ++v)
      {
        centre += mVertices[v].position;
        ++count;
      }

      if (count == 0U) continue;

      centre /= static_cast<float>(count);

      for (uint32_t v = range.first; v <= range.second; ++v)
      {
        // Forward is towards the viewer, which is where z is smallest
        const float forward = -Normalised(mVertices[v].position - centre)[2];

        mVertices[v].material = forward > sPupilCosine
                                  ? static_cast<float>(Material::Pupil)
                                  : (forward > sIrisCosine ? static_cast<float>(Material::Iris)
                                                           : static_cast<float>(Material::EyeWhite));
      }
    }
  }

  void HeadMesh::BuildAdjacency()
  {
    const std::size_t n = mRestPositions.size();

    std::vector<uint32_t> counts(n + 1U, 0U);

    for (std::size_t i = 0U; i + 1U < mEdgeIndices.size(); i += 2U)
    {
      ++counts[mEdgeIndices[i]];
      ++counts[mEdgeIndices[i + 1U]];
    }

    mAdjacencyStart.assign(n + 1U, 0U);

    for (std::size_t v = 0U; v < n; ++v)
    {
      mAdjacencyStart[v + 1U] = mAdjacencyStart[v] + counts[v];
    }

    mAdjacency.assign(mAdjacencyStart[n], 0U);

    std::vector<uint32_t> cursor(mAdjacencyStart.begin(), mAdjacencyStart.end() - 1);

    for (std::size_t i = 0U; i + 1U < mEdgeIndices.size(); i += 2U)
    {
      const uint32_t a = mEdgeIndices[i];
      const uint32_t b = mEdgeIndices[i + 1U];

      mAdjacency[cursor[a]++] = b;
      mAdjacency[cursor[b]++] = a;
    }
  }

  void HeadMesh::GeodesicFrom(uint32_t iVertex, double iCutoff, std::vector<float>& oDistance) const
  {
    constexpr float sUnreached = -1.0F;

    oDistance.assign(mRestPositions.size(), sUnreached);

    if (iVertex >= mRestPositions.size()) return;

    using Entry = std::pair<float, uint32_t>;

    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> queue;

    oDistance[iVertex] = 0.0F;
    queue.emplace(0.0F, iVertex);

    while (!queue.empty())
    {
      const Entry entry = queue.top();
      queue.pop();

      // A vertex can be queued more than once; only the first, shortest, arrival counts
      if (entry.first > oDistance[entry.second]) continue;

      // Nothing beyond the cutoff can shorten anything inside it
      if (entry.first > iCutoff) break;

      for (uint32_t e = mAdjacencyStart[entry.second]; e < mAdjacencyStart[entry.second + 1U]; ++e)
      {
        const uint32_t neighbour = mAdjacency[e];
        const float step = entry.first + Length(mRestPositions[neighbour] - mRestPositions[entry.second]);

        if (step > iCutoff) continue;
        if (oDistance[neighbour] >= 0.0F && oDistance[neighbour] <= step) continue;

        oDistance[neighbour] = step;
        queue.emplace(step, neighbour);
      }
    }
  }

  bool HeadMesh::SolveDeformation(const BuildParams& iParams)
  {
    const int n = static_cast<int>(sLandmarkCount);
    const std::size_t vertices = mRestPositions.size();

    BuildAdjacency();

    // Distance from every landmark to every vertex, along the surface. Walking the mesh is
    // what separates the lips: through space they touch, and no field that is smooth in
    // space can move one without moving the other.
    std::vector<std::vector<float>> geodesic(static_cast<std::size_t>(n));

    for (int j = 0; j < n; ++j)
    {
      GeodesicFrom(static_cast<uint32_t>(mLandmarkVertices[static_cast<std::size_t>(j)]),
                   iParams.reach, geodesic[static_cast<std::size_t>(j)]);
    }

    // How far each landmark reaches, from how close its own neighbours are - measured along
    // the surface as well. A landmark in a crowd influences a small patch; one on its own
    // influences a wide one.
    //
    // The multiplier is pinned by two measurements on this head, and only a surface distance
    // lets both be satisfied at once. The inner lips are 0.59 apart along the surface, so a
    // lip's reach must stay under that or the two will move as one; the nearest brow is 0.64
    // from the forehead, so a brow's reach must exceed that or a raised brow leaves the
    // forehead behind. Against the fourth-nearest landmark - 0.19 at the lips, 0.32 at the
    // brows - 2.2 puts the first at 0.42 and the second at 0.70, and both hold.
    //
    // Through space the same two numbers are 0.009 and 0.64, and nothing satisfies both.
    constexpr int sNeighbour = 4;
    constexpr double sSpread = 2.2;
    constexpr double sShortest = 0.12;

    std::vector<double> support(static_cast<std::size_t>(n));
    std::vector<double> distances;

    for (int j = 0; j < n; ++j)
    {
      distances.clear();

      for (int k = 0; k < n; ++k)
      {
        const float d = geodesic[static_cast<std::size_t>(j)][static_cast<std::size_t>(mLandmarkVertices[static_cast<std::size_t>(k)])];
        if (d >= 0.0F) distances.emplace_back(d);
      }

      double neighbour = iParams.reach;

      if (static_cast<int>(distances.size()) > sNeighbour)
      {
        std::nth_element(distances.begin(), distances.begin() + sNeighbour, distances.end());
        neighbour = distances[sNeighbour];
      }

      support[static_cast<std::size_t>(j)] =
        (std::max)(sShortest, (std::min)(iParams.reach, sSpread * neighbour));
    }

    mWeights.assign(vertices * static_cast<std::size_t>(n), 0.0F);
    mIsDeformed.assign(vertices, false);

    std::vector<double> heard(static_cast<std::size_t>(n));

    for (std::size_t v = 0U; v < vertices; ++v)
    {
      // An eyeball moves as one piece: deforming a sphere by the lids around it would leave
      // it a shape no eye has ever been
      if (mParts[v] == Part::Eyeball) continue;

      double total = 0.0;

      for (int j = 0; j < n; ++j)
      {
        const float d = geodesic[static_cast<std::size_t>(j)][v];

        heard[static_cast<std::size_t>(j)] =
          d < 0.0F ? 0.0 : Weight(d, support[static_cast<std::size_t>(j)]);

        total += heard[static_cast<std::size_t>(j)];
      }

      // No landmark reaches this vertex, so nothing the face does can move it
      if (total <= 0.0) continue;

      float* row = &mWeights[v * static_cast<std::size_t>(n)];

      for (int j = 0; j < n; ++j)
      {
        row[j] = static_cast<float>(heard[static_cast<std::size_t>(j)] / total);
      }

      mIsDeformed[v] = true;
    }

    return true;
  }

  void HeadMesh::BakeOcclusion()
  {
    // How concave the surface is around each vertex: the average of its neighbours sits in
    // front of a hollow and behind a bulge. It is not a light simulation, but it darkens the
    // eye sockets, the nostrils and the crease of the lips, which is what it is for.
    std::vector<cv::Vec3f> neighbourSum(mVertices.size(), cv::Vec3f(0.0F, 0.0F, 0.0F));
    std::vector<float> neighbourCount(mVertices.size(), 0.0F);

    for (std::size_t i = 0U; i + 1U < mEdgeIndices.size(); i += 2U)
    {
      const uint32_t a = mEdgeIndices[i];
      const uint32_t b = mEdgeIndices[i + 1U];

      neighbourSum[a] += mRestPositions[b];
      neighbourSum[b] += mRestPositions[a];

      neighbourCount[a] += 1.0F;
      neighbourCount[b] += 1.0F;
    }

    // Normals are not built yet, so the outward direction is taken from the head's middle.
    // Close enough for an occlusion term, and it needs no second pass.
    cv::Vec3f centre(0.0F, 0.0F, 0.0F);
    for (const auto& position : mRestPositions) centre += position;

    centre /= static_cast<float>(mRestPositions.size());

    for (std::size_t v = 0U; v < mVertices.size(); ++v)
    {
      if (neighbourCount[v] < 1.0F) continue;

      const cv::Vec3f average = neighbourSum[v] / neighbourCount[v];
      const cv::Vec3f outward = Normalised(mRestPositions[v] - centre);

      // Positive when the neighbours are further out than this vertex, which is a hollow
      const float hollow = Dot(average - mRestPositions[v], outward);

      mVertices[v].occlusion = (std::max)(0.45F, (std::min)(1.0F, 1.0F - hollow * 26.0F));
    }
  }

  void HeadMesh::AlignToNeutral(std::vector<cv::Vec3f>& ioShape) const
  {
    if (ioShape.size() != mNeutralLandmarks.size()) return;

    // The pipeline aligns its normalised shape onto the mean its own alignment converged on,
    // which is close to the canonical model but is not it. The last turn is taken here, or a
    // residual rotation would read as a head that is permanently tilted.
    cv::Matx33d covariance = cv::Matx33d::zeros();

    for (std::size_t i = 0U; i < ioShape.size(); ++i)
    {
      for (int r = 0; r < 3; ++r)
      {
        for (int c = 0; c < 3; ++c)
        {
          covariance(r, c) += static_cast<double>(ioShape[i][r]) * static_cast<double>(mNeutralLandmarks[i][c]);
        }
      }
    }

    cv::Matx33d u, vt;
    cv::Matx31d w;

    cv::SVD::compute(covariance, w, u, vt);

    cv::Matx33d rotation = vt.t() * u.t();

    // A shape that is nearly a mirror of the model would otherwise be reflected onto it,
    // which turns the head inside out
    if (cv::determinant(rotation) < 0.0)
    {
      cv::Matx33d flip = cv::Matx33d::eye();
      flip(2, 2) = -1.0;

      rotation = vt.t() * flip * u.t();
    }

    for (auto& point : ioShape)
    {
      const cv::Matx31d turned = rotation * cv::Matx31d(point[0], point[1], point[2]);

      point = { static_cast<float>(turned(0)), static_cast<float>(turned(1)), static_cast<float>(turned(2)) };
    }
  }

  void HeadMesh::Drive(const DriveInput& iInput)
  {
    if (!IsBuilt()) return;

    const float keep = static_cast<float>((std::max)(0.0, (std::min)(0.95, iInput.smoothing)));

    const bool hasShape = iInput.normalisedShape != nullptr &&
                          ToMeshSpace(*iInput.normalisedShape, mIncomingLandmarks) &&
                          mIncomingLandmarks.size() == mNeutralLandmarks.size();

    if (hasShape)
    {
      AlignToNeutral(mIncomingLandmarks);

      // The first frame of a face snaps into place; from then on the shape is eased, which
      // is what turns a per-frame fit into a motion. A shape the caller has already passed
      // is not eased again, or how smooth the head looked would depend on the frame rate.
      const float blend = !mHasDrivenShape ? 1.0F : (iInput.shapeIsNew ? (1.0F - keep) : 0.0F);

      for (std::size_t i = 0U; i < mDrivenLandmarks.size(); ++i)
      {
        mDrivenLandmarks[i] += (mIncomingLandmarks[i] - mDrivenLandmarks[i]) * blend;
      }

      mHasDrivenShape = true;
    }
    else
    {
      // Nothing to follow: ease back to the average face, which leaves the authored head
      const float blend = 1.0F - keep;

      for (std::size_t i = 0U; i < mDrivenLandmarks.size(); ++i)
      {
        mDrivenLandmarks[i] += (mNeutralLandmarks[i] - mDrivenLandmarks[i]) * blend;
      }
    }

    ApplyDisplacement(iInput.expression);
    PlaceEyeballs();
    RecomputeNormals();
  }

  void HeadMesh::ApplyDisplacement(double iExpression)
  {
    const float expression = static_cast<float>((std::max)(0.0, (std::min)(3.0, iExpression)));
    const std::size_t n = sLandmarkCount;

    mLandmarkDeltas.resize(n);

    // How far this face departs from the average one - which is both what it is doing and
    // whose face it is - applied to a head that has its own identity to begin with
    for (std::size_t l = 0U; l < n; ++l)
    {
      mLandmarkDeltas[l] = (mDrivenLandmarks[l] - mNeutralLandmarks[l]) * expression;
    }

    for (std::size_t v = 0U; v < mVertices.size(); ++v)
    {
      if (!mIsDeformed[v])
      {
        mVertices[v].position = mRestPositions[v];
        continue;
      }

      const float* row = &mWeights[v * n];

      cv::Vec3f displacement(0.0F, 0.0F, 0.0F);

      for (std::size_t j = 0U; j < n; ++j)
      {
        displacement += mLandmarkDeltas[j] * row[j];
      }

      mVertices[v].position = mRestPositions[v] + displacement;
    }
  }

  void HeadMesh::PlaceEyeballs()
  {
    if (mEyeballRanges.empty()) return;

    // Each eyeball follows its own eye: the average of how far that eye's six landmarks
    // moved, applied to the whole sphere so it slides rather than deforms
    const std::pair<Landmark, Landmark> eyes[2] = {
      { Landmark::kRightEye0, Landmark::kRightEye5 },
      { Landmark::kLeftEye0, Landmark::kLeftEye5 }
    };

    for (std::size_t e = 0U; e < mEyeballRanges.size(); ++e)
    {
      const auto& eye = eyes[(std::min)(e, std::size_t{ 1U })];

      cv::Vec3f shift(0.0F, 0.0F, 0.0F);
      int count = 0;

      for (int l = Index(eye.first); l <= Index(eye.second); ++l)
      {
        shift += mLandmarkDeltas[static_cast<std::size_t>(l)];
        ++count;
      }

      if (count > 0) shift /= static_cast<float>(count);

      for (uint32_t v = mEyeballRanges[e].first; v <= mEyeballRanges[e].second; ++v)
      {
        mVertices[v].position = mRestPositions[v] + shift;
      }
    }
  }

  void HeadMesh::RecomputeNormals()
  {
    for (auto& vertex : mVertices)
    {
      vertex.normal = { 0.0F, 0.0F, 0.0F };
    }

    // Area-weighted, which falls out of not normalising the cross product before adding it
    for (std::size_t i = 0U; i + 2U < mIndices.size(); i += 3U)
    {
      const uint32_t ia = mIndices[i];
      const uint32_t ib = mIndices[i + 1U];
      const uint32_t ic = mIndices[i + 2U];

      const cv::Vec3f faceNormal = Cross(mVertices[ib].position - mVertices[ia].position,
                                         mVertices[ic].position - mVertices[ia].position);

      mVertices[ia].normal += faceNormal;
      mVertices[ib].normal += faceNormal;
      mVertices[ic].normal += faceNormal;
    }

    for (auto& vertex : mVertices)
    {
      vertex.normal = Normalised(vertex.normal);
    }
  }
}
