#include "Graph/Graph.h"
#include "Common/Configuration.h"

namespace face
{
	Graph::Graph(const std::string& iName) :
		fw::Module(iName),
		mExecutor(fw::getInlineExecutor())
	{
	}

	Graph::~Graph()
	{
		DeInitialize();
	}

	void Graph::Clear()
	{
		for (auto& module : mModules)
			module->Clear();
	}

	fw::ErrorCode Graph::Insert(fw::Module* iModule)
	{
		if (iModule == nullptr)
			return fw::ErrorCode::NotFound;

		const cv::FileNode moduleSettings = Configuration::GetInstance().GetModuleSettings(iModule->GetName());
		const fw::ErrorCode result = iModule->Initialize(moduleSettings);

		mModules.push_back(iModule);

		return result;
	}

	fw::ErrorCode Graph::InitializeInternal(const cv::FileNode& /*iSettingsNode*/)
	{
		return fw::ErrorCode::OK;
	}

	fw::ErrorCode Graph::DeInitializeInternal()
	{
		for (auto& module : mModules)
		{
			module->DeInitialize();
			delete module; module = nullptr;
		}

		mModules.clear();

		return fw::ErrorCode::OK;
	}
}
