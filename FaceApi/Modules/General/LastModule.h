#pragma once

#include "Modules/General/ModuleWithPort.hpp"

#include <functional>

namespace face
{
	class LastModule :
		public ModuleWithPort<bool(bool)>
	{
	public:
		LastModule() = default;

		virtual ~LastModule() = default;

		bool Main(bool iSucceeded) override;

		bool Get() const;

		void Wait() const;
	};
}
