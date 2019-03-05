#pragma once

#include <string>
#include <vector>

#include "Framework/Module.h"
#include "Framework/FlowGraph.hpp"

namespace face
{
	class Graph :
		public fw::Module
	{
	public:
		explicit Graph(const std::string& iName);

		virtual ~Graph();

		void Clear() override;

	protected:
		fw::ErrorCode Insert(fw::Module* iModule);

		virtual fw::ErrorCode Connect() = 0;

		fw::Executor::Shared mExecutor = nullptr;
		fw::FirstNode<unsigned> mFirstNode;
		fw::LastNode<bool> mLastNode;

	private:
		Graph(const Graph& iOther) = delete;

		Graph& operator=(const Graph& iOther) = delete;
		
		fw::ErrorCode InitializeInternal(const cv::FileNode& iSettingsNode) override;

		fw::ErrorCode DeInitializeInternal() override;

		std::vector<fw::Module*> mModules;
	};
}
