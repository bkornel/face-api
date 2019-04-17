#pragma once

#include "Framework/Message.h"

#include <opencv2/core/core.hpp>

namespace face
{
	class RoiMessage :
		public fw::Message
	{
	public:
		FW_DEFINE_SMART_POINTERS(RoiMessage)

		RoiMessage(const std::vector<cv::Rect>& iROIs, unsigned iFrameId, long long iTimestamp);

		virtual ~RoiMessage() = default;

		friend inline std::ostream& operator<<(std::ostream& ioStream, const RoiMessage& iMessage);

		inline bool IsEmpty() const
		{
			return mROIs.empty();
		}

		inline std::size_t GetSize() const
		{
			return mROIs.size();
		}

		inline const std::vector<cv::Rect>& GetROIs() const
		{
			return mROIs;
		}

	private:
		std::vector<cv::Rect> mROIs;
	};

	inline std::ostream& operator<< (std::ostream& ioStream, const RoiMessage& iMessage)
	{
		const fw::Message& base(iMessage);
		ioStream << base << ", [Derived] Size of ROIs: " << iMessage.GetSize();
		return ioStream;
	}
}
