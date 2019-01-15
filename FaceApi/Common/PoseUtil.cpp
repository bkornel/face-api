#include "PoseUtil.h"

namespace face
{
	PoseUtil& PoseUtil::GetInstance()
	{
		static PoseUtil sInstance;
		return sInstance;
	}

	PoseUtil::PoseUtil() :
		mOrigin3D(0.0, 0.0, 0.0)
	{
		mAxes3D =
		{
			mOrigin3D,
			{ 100, 0, 0 },
			{ 0, 100, 0 },
			{ 0, 0, 100 }
		};

		mUnitBox =
		{
			// Front face
			{ 0.0, 0.0, 0.0 },	// A
			{ 1.0, 0.0, 0.0 },	// B
			{ 0.0, 1.0, 0.0 },	// C
			{ 1.0, 1.0, 0.0 },	// D

								// Rear face
			{ 0.0, 0.0, 1.0 },	// E
			{ 1.0, 0.0, 1.0 },	// F
			{ 0.0, 1.0, 1.0 },	// G
			{ 1.0, 1.0, 1.0 }	// H
		};

		mConnections =
		{
			// Front face
			{ 0, 1 },
			{ 1, 3 },
			{ 3, 2 },
			{ 2, 0 },

			// Rear face
			{ 4, 5 },
			{ 5, 7 },
			{ 7, 6 },
			{ 6, 4 },

			// Sides
			{ 0, 4 },
			{ 1, 5 },
			{ 2, 6 },
			{ 3, 7 }
		};
	}
}
