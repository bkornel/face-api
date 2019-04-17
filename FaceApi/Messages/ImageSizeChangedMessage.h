#pragma once

#include "Framework/Message.h"

#include <opencv2/core/core.hpp>

namespace face
{
	class ImageSizeChangedMessage :
		public fw::Message
	{
	public:
		FW_DEFINE_SMART_POINTERS(ImageSizeChangedMessage)

		ImageSizeChangedMessage(const cv::Size& iSize, unsigned iFrameId, long long iTimestamp);

		virtual ~ImageSizeChangedMessage() = default;

		friend inline std::ostream& operator<<(std::ostream& ioStream, const ImageSizeChangedMessage& iMessage);

		inline bool IsEmpty() const
		{
			return mSize.empty();
		}

		inline int GetWidth() const
		{
			return mSize.width;
		}

		inline int GetHeight() const
		{
			return mSize.height;
		}

		inline const cv::Size& GetSize() const
		{
			return mSize;
		}

	private:
		cv::Size mSize;
	};

	inline std::ostream& operator<< (std::ostream& ioStream, const ImageSizeChangedMessage& iMessage)
	{
		const fw::Message& base(iMessage);
		ioStream << base << ", [Derived] Width: " << iMessage.GetWidth() << ", Height: " << iMessage.GetHeight();
		return ioStream;
	}
}
