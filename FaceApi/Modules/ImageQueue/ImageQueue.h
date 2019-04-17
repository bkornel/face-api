#pragma once

#include <string>

#include "Common/Configuration.h"
#include "Framework/Module.h"
#include "Framework/MessageQueue.hpp"
#include "Framework/FlowGraph.hpp"
#include "Messages/ImageMessage.h"

namespace face
{
	using ImageQueueBase = fw::ModuleWithPort<ImageMessage::Shared(unsigned)>;

	class ImageQueue :
		public ImageQueueBase
	{
		using MessageQueue = fw::MessageQueue<ImageMessage::Shared>;

	public:
		ImageQueue();

		virtual ~ImageQueue() = default;

		fw::ErrorCode Push(const cv::Mat& iFrame);

		ImageMessage::Shared Main(unsigned iTickNumber) override;

		void Clear() override
		{
			mQueue.Clear();
		}

		inline const cv::Size& GetImageSize() const
		{
			return mImageSize;
		}

		inline unsigned GetLastFrameId() const
		{
			return mLastFrameId;
		}

		inline long long GetLastTimestamp() const
		{
			return mLastTimestamp;
		}

		inline int GetQueueSize() const
		{
			return mQueue.GetSize();
		}

		inline float GetSamplingFPS() const
		{
			return mQueue.GetSamplingFPS();
		}

		inline int GetBound() const
		{
			return mQueue.GetBound();
		}

	private:
		fw::ErrorCode InitializeInternal(const cv::FileNode& iSettings) override;

		unsigned mLastFrameId = 0U;		///< Holds the ID of the last image frame.
		long long mLastTimestamp = 0;
		MessageQueue mQueue;			///< Queue for handling the frames
		cv::Size mImageSize;
	};
}
