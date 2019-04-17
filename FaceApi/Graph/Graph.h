#pragma once

#include "Framework/Module.h"
#include "Framework/FlowGraph.hpp"
#include "Modules/General/FirstModule.h"
#include "Modules/General/LastModule.h"

#include <map>
#include <string>
#include <vector>

namespace face
{
	class Graph :
		public fw::Module
	{
		using PredecessorMap = std::map<int, std::string>;

	public:
		Graph();

		virtual ~Graph();

		void Clear() override;

	private:
		Graph(const Graph& iOther) = delete;

		Graph& operator=(const Graph& iOther) = delete;

		fw::ErrorCode InitializeInternal(const cv::FileNode& iModulesNode) override;

		fw::ErrorCode DeInitializeInternal() override;

		fw::ErrorCode CreateModules(const cv::FileNode& iModulesNode);

		fw::ErrorCode CreateConnections(const cv::FileNode& iModulesNode);

		fw::ErrorCode GetPredecessors(const cv::FileNode& iModule, const cv::FileNode& iModules, PredecessorMap& oPredecessors);

		FirstModule::Shared mFirstNode = nullptr;
		LastModule::Shared mLastNode = nullptr;
		fw::Executor::Shared mExecutor = nullptr;
		std::vector<fw::Module::Shared> mModules;
	};
}
