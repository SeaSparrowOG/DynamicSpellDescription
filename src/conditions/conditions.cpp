#include "conditions/conditions.h"

namespace Conditions
{
	bool Cache::AddCondition(RE::SpellItem* a_target, ComplexCondition a_condition)
	{
		if (this->spellConditions.contains(a_target)) {
			spellConditions[a_target].push_back(a_condition);
		}
		else {
			std::vector<ComplexCondition> newVec{};
			newVec.push_back(a_condition);
			spellConditions.emplace(a_target, newVec);
		}
		return true;
	}

	std::string Cache::GetSpellDescription(RE::SpellItem* a_spell)
	{
		if (spellConditions.contains(a_spell)) {
			const auto& conditions = spellConditions[a_spell];
			std::string newDescription = "";
			for (auto condition : conditions) {
				newDescription += condition.IsValid() ? condition.newText : "";
			}
			return newDescription;
		}
		return "";
	}

	bool Cache::ComplexCondition::IsValid()
	{
		const auto player = RE::PlayerCharacter::GetSingleton();

		bool hasRequiredPerk = false;
		for (auto it = requiredANDPerks.begin(); !hasRequiredPerk && it != requiredANDPerks.end(); ++it) {
			if (!player->HasPerk(*it)) {
				hasRequiredPerk = true;
			}
		}
		if (hasRequiredPerk) {
			return false;
		}

		bool hasExcludedPerk = false;
		for (auto it = excludedANDPerks.begin(); !hasExcludedPerk && it != excludedANDPerks.end(); ++it) {
			if (player->HasPerk(*it)) {
				hasExcludedPerk = true;
			}
		}
		if (hasExcludedPerk) {
			return false;
		}

		bool hasOptionalPerk = false;
		for (auto it = requiredORPerks.begin(); !hasOptionalPerk && it != requiredORPerks.end(); ++it) {
			if (player->HasPerk(*it)) {
				hasExcludedPerk = true;
			}
		}
		if (!hasOptionalPerk) {
			return false;
		}

		hasOptionalPerk = false;
		for (auto it = excludedORPerks.begin(); !hasOptionalPerk && it != excludedORPerks.end(); ++it) {
			if (player->HasPerk(*it)) {
				hasExcludedPerk = true;
			}
		}
		if (hasExcludedPerk) {
			return false;
		}

		return false;
	}
}