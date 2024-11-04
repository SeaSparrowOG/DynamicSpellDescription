#pragma once

#include "utilities/utilities.h"

namespace Conditions
{
	class Cache : public Utilities::Singleton::ISingleton<Cache> {
	public:
		struct ComplexCondition {
			bool IsValid();

			std::string_view newText;
			std::vector<RE::BGSPerk*> requiredORPerks;
			std::vector<RE::BGSPerk*> requiredANDPerks;
			std::vector<RE::BGSPerk*> excludedORPerks;
			std::vector<RE::BGSPerk*> excludedANDPerks;
		};

		bool AddCondition(RE::SpellItem* a_target, ComplexCondition a_condition);
		std::string GetSpellDescription(RE::SpellItem* a_spell);

	private:
		std::unordered_map<RE::SpellItem*, std::vector<ComplexCondition>> spellConditions;
	};
}