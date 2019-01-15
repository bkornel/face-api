#define _USE_MATH_DEFINES

#include <cmath>
#include <opencv2/highgui/highgui.hpp>
#include <easyloggingpp/easyloggingpp.h>

#include "Common/Configuration.h"
#include "Framework/UtilString.h"
#include "Framework/UtilOCV.h"

namespace face
{
	Configuration& Configuration::GetInstance()
	{
		static Configuration sInstance;
		return sInstance;
	}

	fw::ErrorCode Configuration::Initialize(const std::string& iConfigFile)
	{
		static bool sInitialized = false;

		if (sInitialized) return fw::ErrorCode::OK;

		sInitialized = true;

		std::string jsonFile = mDirectories.working + iConfigFile;
		std::replace(jsonFile.begin(), jsonFile.end(), '\\', '/');

		if (!mFileStorage.open(jsonFile, cv::FileStorage::READ))
		{
			LOG(ERROR) << "ERROR: " << jsonFile << " cannot be opened.";
			sInitialized = false;
			return fw::ErrorCode::BadData;
		}

		const cv::FileNode& faceNode = mFileStorage["face"];
		if (!faceNode.empty())
		{
			const cv::FileNode& generalNode = faceNode["general"];
			if (!generalNode.empty())
			{
				if (LoadSettings(generalNode))
				{
					const std::string& configFile = mDirectories.working + "log.cfg";
					const std::string& logFile = mDirectories.output + fw::get_log_stamp() + ".txt";

					el::Configurations conf(configFile);
					conf.set(el::Level::Global, el::ConfigurationType::Filename, logFile);
					el::Loggers::reconfigureAllLoggers(conf);
				}
				else
				{
					sInitialized = false;
				}
			}

			mModulesNode = faceNode["modules"];
		}

		return sInitialized ? fw::ErrorCode::OK : fw::ErrorCode::SystemFailure;
	}

	bool Configuration::LoadSettings(const cv::FileNode& iGeneralNode)
	{
		std::string value;

		const cv::FileNode& outputNode = iGeneralNode["output"];
		if (!outputNode.empty())
		{
			if (fw::ocv::get_value(outputNode, "verbose", value))
				mOutput.verbose = fw::str::convert_to_boolean(value);

			if (fw::ocv::get_value(outputNode, "video", value))
				mOutput.video = fw::str::convert_to_boolean(value);

			if (fw::ocv::get_value(outputNode, "videoFPS", value))
				mOutput.videoFPS = fw::str::convert_to_number<float>(value);

			if (fw::ocv::get_value(outputNode, "videoFourCC", value) && (value.size() == 4))
				mOutput.videoFourCC = cv::VideoWriter::fourcc(value[0], value[1], value[2], value[3]);
		}

		const cv::FileNode& dirNode = iGeneralNode["directories"];
		if (!dirNode.empty())
		{
			if (fw::ocv::get_value(dirNode, "faceDetector", value))
				mDirectories.faceDetector = value;

			if (fw::ocv::get_value(dirNode, "shapeModel", value))
				mDirectories.shapeModel = value;

			if (fw::ocv::get_value(dirNode, "output", value))
				mDirectories.output = value;
		}

		RebuildPaths();

		return true;
	}

	cv::FileNode Configuration::GetModuleSettings(const std::string& iName) const
	{
		static const cv::FileNode sEmptyNode;

		if (!mModulesNode.empty())
		{
			const std::string& lower1 = fw::str::to_lower(iName);

			for (const auto& m : mModulesNode)
			{
				if (m.empty() || !m.isNamed()) continue;

				const std::string& lower2 = fw::str::to_lower(m.name());

				if (lower1 == lower2) return m;
			}
		}

		LOG(WARNING) << "Settings for " << iName << " cannot be found.";

		return sEmptyNode;
	}

	void Configuration::SetWorkingDirectory(const std::string& iWorkingDir)
	{
		mDirectories.working = iWorkingDir;
		RebuildPaths();
	}

	void Configuration::RebuildPaths()
	{
		FixPathSeparator(mDirectories.working);

		mDirectories.faceDetector = mDirectories.working + mDirectories.faceDetector;
		FixPathSeparator(mDirectories.faceDetector);

		mDirectories.shapeModel = mDirectories.working + mDirectories.shapeModel;
		FixPathSeparator(mDirectories.shapeModel);

		mDirectories.output = mDirectories.working + mDirectories.output;
		FixPathSeparator(mDirectories.output);
	}

	void Configuration::FixPathSeparator(std::string& ioPath)
	{
		std::replace(ioPath.begin(), ioPath.end(), '\\', '/');
		if (!fw::str::ends_with(ioPath, "/") && !fw::str::ends_with(ioPath, "\\"))
			ioPath += "/";
	}
}
