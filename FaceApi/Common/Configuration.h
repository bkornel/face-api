#pragma once

#include <vector>
#include <string>
#include <opencv2/core/core.hpp>
#include <opencv2/videoio/videoio.hpp>

#include "Framework/Util.h"

namespace face
{
	struct OutputParams
	{
		bool verbose = false;
		bool video = false;
		float videoFPS = 15.0F;
		int videoFourCC = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
	};

	struct DirectoryParams
	{
		std::string working;
		std::string faceDetector = "faceDetector/";
		std::string shapeModel = "shapeModel/";
		std::string output = "output/";
	};

	class Configuration
	{
	public:
		static Configuration& GetInstance();

		fw::ErrorCode Initialize(const std::string& iConfigFile = "settings.json");

		inline const DirectoryParams& GetDirectories() const { return mDirectories; }

		inline const OutputParams& GetOutput() const { return mOutput; }

		inline const cv::FileNode& GetModulesNode() const { return mModulesNode; }

		cv::FileNode GetModuleSettings(const std::string& iName) const;

		void SetWorkingDirectory(const std::string& iWorkingDir);

		inline void SetVerboseMode(bool iVerboseMode)
		{
			mOutput.verbose = iVerboseMode;
		}

	private:
		Configuration() = default;

		Configuration(const Configuration& iOther) = delete;

		Configuration& operator=(const Configuration& iOther) = delete;

		bool LoadSettings(const cv::FileNode& iGeneralNode);

		void RebuildPaths();

		void FixPathSeparator(std::string& ioPath);

		cv::FileStorage mFileStorage;
		cv::FileNode mModulesNode;

		DirectoryParams mDirectories;
		OutputParams mOutput;
	};
}
