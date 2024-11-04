#pragma once

#include "utilities/utilities.h"

namespace Conditions
{
	class Cache : public Utilities::Singleton::ISingleton<Cache> {
	public:
		struct ComplexCondition {

		};

		bool AddCondition(RE::SpellItem* a_target, const ComplexCondition& a_condition, const std::string_view& newText);
	};
}