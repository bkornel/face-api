#pragma once

#include "Modules/General/ModuleWithPort.hpp"

#include <functional>

namespace face
{
	class FirstModule :
		public ModuleWithPort<unsigned(bool)>
	{
	public:
		FirstModule() = default;

		virtual ~FirstModule() = default;

		void Connect(fw::Executor::Shared iExecutor);

		unsigned Main(bool);

		void Tick();

	private:
		void Connect() override;

		std::function<void()> mFunction;
	};
}
