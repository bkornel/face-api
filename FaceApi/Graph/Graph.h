#pragma once

#include <map>
#include <string>
#include <vector>

#include "Framework/Module.h"
#include "Framework/FlowGraph.hpp"

namespace face
{
	class Graph :
		public fw::Module
	{
		using ConnectionMap = std::map<int, std::string>;

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

		fw::ErrorCode GetPredecessors(const std::string& iModuleName, const cv::FileNode& iModulesNode, ConnectionMap& oConnectionMap);

		fw::Executor::Shared mExecutor = nullptr;
		fw::FirstNode<unsigned> mFirstNode;
		fw::LastNode<bool> mLastNode;

		std::vector<fw::Module::Shared> mModules;
	};
}
