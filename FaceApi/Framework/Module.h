#pragma once

#include <string>
#include <memory>
#include <opencv2/core.hpp>

#include "Thread.h"
#include "UtilOCV.h"
#include "UtilString.h"

namespace fw
{
	class Module :
		public Thread
	{
	public:
		explicit Module(const std::string& iName);

		virtual ~Module() = default;

		virtual ErrorCode Initialize(const cv::FileNode& iSettings);

		virtual ErrorCode DeInitialize();

		virtual void Clear()
		{
		}

		inline bool IsInitialized() const
		{
			return mInitialized;
		}

		inline const std::string& GetName() const
		{
			return mName;
		}

	protected:
		virtual ErrorCode InitializeInternal(const cv::FileNode& iSettings)
		{
			return fw::ErrorCode::OK;
		}

		virtual ErrorCode DeInitializeInternal()
		{
			return fw::ErrorCode::OK;
		}

		bool mInitialized = false;

		std::string mName;
	};
}