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

		Graph(const Graph& iOther) = delete;

		virtual ~Graph() = default;

		Graph& operator=(const Graph& iOther) = delete;

		void Clear() override;

	protected:
		virtual fw::ErrorCode Connect() = 0;

		fw::ErrorCode DeInitializeInternal() override;

		fw::ErrorCode Insert(fw::Module* iModule);

		fw::Executor::Shared mExecutor = nullptr;
		fw::FirstNode<unsigned> mFirstNode;
		fw::LastNode<bool> mLastNode;

	private:		
		std::vector<fw::Module*> mModules;
	};
}
