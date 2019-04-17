#pragma once

#include "Framework/UtilOCV.h"

#include <opencv2/core/core.hpp>

#include <vector>

namespace face
{
	class PoseUtil
	{
	public:
		using Connections = std::vector<std::pair<int, int>>;

		static PoseUtil& GetInstance();

		PoseUtil();

		inline const cv::Point3d& GetOrigin3D() const
		{
			return mOrigin3D;
		}

		inline const fw::ocv::VectorPt3D& GetAxes3D() const
		{
			return mAxes3D;
		}

		inline const fw::ocv::VectorPt3D& GetUnitBox() const
		{
			return mUnitBox;
		}

		//  E---------F
		// /|        /|
		// A--------B |
		// | G------|-H
		// |/       |/
		// C--------D
		inline const Connections& GetConnections() const
		{
			return mConnections;
		}

	private:
		~PoseUtil() = default;

		const cv::Point3d mOrigin3D;

		fw::ocv::VectorPt3D mAxes3D;
		fw::ocv::VectorPt3D mUnitBox;
		Connections mConnections;
	};
}
