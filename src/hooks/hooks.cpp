#include "Hooks/hooks.h"

#include "utilities/effectEvaluator.h"

namespace Hooks {
	void Install()
	{
		SKSE::AllocTrampoline(70);
		SpellItemDescription::Install();
	}

	void SpellItemDescription::RegisterNewEffectDescription(RE::EffectSetting* a_effect, std::string& a_newDescription)
	{
		std::string::const_iterator searchStart(a_newDescription.cbegin());
		std::smatch matches;
		std::string response;

		while (std::regex_search(searchStart, a_newDescription.cend(), matches, pattern)) {
			response += matches.prefix().str();
			response += openingTag;
			auto contents = matches[1].str();
			auto contentsLower = Utilities::String::tolower(contents);
			if (contentsLower.starts_with("global=")) {
				auto globalEDID = contents.substr(7);
				const auto global = RE::TESForm::LookupByEditorID<RE::TESGlobal>(globalEDID);
				if (global) {
					response += std::to_string(global->value);
				}
			}
			else {
				response += contents;
			}
			response += closingTag;
			searchStart = matches.suffix().first;
		}
		response += std::string(searchStart, a_newDescription.cend());

		specialEffects[a_effect] = response;
	}

	void SpellItemDescription::GetSpellDescription(RE::ItemCard* a1, RE::SpellItem* a2, RE::BSString& a_out)
	{
		_getSpellDescription(a1, a2, a_out);

		const auto player = RE::PlayerCharacter::GetSingleton();
		const auto singleton = SpellItemDescription::GetSingleton();
		assert(player && singleton);
		if (!player || !singleton) {
			logger::error("Failed to get the player or SpellItemDescirptionSingleton. This will likely cause a crash later.");
			return;
		}

		auto* frontEffect = a2->effects.front();
		auto* frontBaseEffect = frontEffect ? frontEffect->baseEffect : nullptr;
		if (!frontBaseEffect) {
			logger::error("Failed to get the front effect of {}.", a2->GetName());
			return;
		}

		auto params = RE::ConditionCheckParams(player, player);
		bool checkAll = EffectEvaluator::ShouldCheckAllConditions(frontBaseEffect);
		std::string newDescription = "";
		for (const auto effect : a2->effects) {
			const auto baseEffect = effect ? effect->baseEffect : nullptr;
			if (!baseEffect) {
				continue;
			}
			if (baseEffect->data.flags.any(RE::EffectSetting::EffectSettingData::Flag::kHideInUI)) {
				continue;
			}
			
			if (EffectEvaluator::AreConditionsValid(effect, params, player, checkAll)) {
				newDescription += singleton->GetFormattedEffectDescription(effect);
			}
		}

		if (newDescription.empty()) {
			return;
		}
		a_out = newDescription;
	}

	void SpellItemDescription::GetSpellTomeDescription(RE::ItemCard* a1, RE::SpellItem* a2, RE::BSString& a_out)
	{
		_getSpellTomeDescription(a1, a2, a_out);

		const auto player = RE::PlayerCharacter::GetSingleton();
		const auto singleton = SpellItemDescription::GetSingleton();
		assert(player && singleton);
		if (!player || !singleton) {
			logger::error("Failed to get the player or SpellItemDescirptionSingleton. This will likely cause a crash later.");
			return;
		}

		auto* frontEffect = a2->effects.front();
		auto* frontBaseEffect = frontEffect ? frontEffect->baseEffect : nullptr;
		if (!frontBaseEffect) {
			logger::error("Failed to get the front effect of {}.", a2->GetName());
			return;
		}

		auto params = RE::ConditionCheckParams(player, player);
		bool checkAll = EffectEvaluator::ShouldCheckAllConditions(frontBaseEffect);
		std::string newDescription = "";
		for (const auto effect : a2->effects) {
			const auto baseEffect = effect ? effect->baseEffect : nullptr;
			if (!baseEffect) {
				continue;
			}
			if (baseEffect->data.flags.any(RE::EffectSetting::EffectSettingData::Flag::kHideInUI)) {
				continue;
			}

			if (EffectEvaluator::AreConditionsValid(effect, params, player, checkAll)) {
				newDescription += singleton->GetFormattedEffectDescription(effect);
			}
		}

		if (newDescription.empty()) {
			return;
		}
		a_out = newDescription;
	}

	void SpellItemDescription::GetMagicWeaponDescription(RE::ItemCard* a1, RE::SpellItem* a2, RE::BSString& a_out)
	{
		_getMagicWeaponDescription(a1, a2, a_out);

		const auto player = RE::PlayerCharacter::GetSingleton();
		const auto singleton = SpellItemDescription::GetSingleton();
		assert(player && singleton);
		if (!player || !singleton) {
			logger::error("Failed to get the player or SpellItemDescirptionSingleton. This will likely cause a crash later.");
			return;
		}

		auto* frontEffect = a2->effects.front();
		auto* frontBaseEffect = frontEffect ? frontEffect->baseEffect : nullptr;
		if (!frontBaseEffect) {
			logger::error("Failed to get the front effect of {}.", a2->GetName());
			return;
		}

		auto params = RE::ConditionCheckParams(player, player);
		bool checkAll = EffectEvaluator::ShouldCheckAllConditions(frontBaseEffect);
		std::string newDescription = "";
		for (const auto effect : a2->effects) {
			const auto baseEffect = effect ? effect->baseEffect : nullptr;
			if (!baseEffect) {
				continue;
			}
			if (baseEffect->data.flags.any(RE::EffectSetting::EffectSettingData::Flag::kHideInUI)) {
				continue;
			}

			if (EffectEvaluator::AreConditionsValid(effect, params, player, checkAll)) {
				newDescription += singleton->GetFormattedEffectDescription(effect);
			}
		}

		if (newDescription.empty()) {
			return;
		}
		a_out = newDescription;
	}

	RE::EffectSetting* SpellItemDescription::ProcessProjectile(RE::MagicItem* a_this)
	{
		auto response = _processProjectile(a_this);
		if (!response || !response->data.projectileBase) {
			return response;
		}

		return EffectEvaluator::GetMostEffectValidEffect(a_this);
	}

	RE::EffectSetting* SpellItemDescription::ProcessImpact(RE::MagicItem* a_this)
	{
		auto response = _processImpact(a_this);
		if (!response || !response->data.impactDataSet) {
			return response;
		}

		return EffectEvaluator::GetMostEffectValidEffect(a_this);
	}

	std::string SpellItemDescription::GetFormattedEffectDescription(RE::Effect* a_effect)
	{
		const auto baseEffect = a_effect->baseEffect;
		if (specialEffects.contains(baseEffect)) {
			return specialEffects[baseEffect];
		}

		std::string originalDescription = baseEffect->magicItemDescription.c_str();
		if (originalDescription.empty()) {
			return originalDescription;
		}

		std::string::const_iterator searchStart(originalDescription.cbegin());
		std::smatch matches;
		std::string response;

		while (std::regex_search(searchStart, originalDescription.cend(), matches, pattern)) {
			response += matches.prefix().str();
			response += openingTag;
			auto contents = matches[1].str();
			auto contentsLower = Utilities::String::tolower(contents);
			if (contentsLower == "mag") {
				int64_t clampedValue = std::clamp(
					static_cast<int64_t>(a_effect->GetMagnitude()),
					std::numeric_limits<int64_t>::min(),
					std::numeric_limits<int64_t>::max()
				);
				response += std::to_string(clampedValue);
			}
			else if (contentsLower == "dur") {
				response += std::to_string(a_effect->GetDuration());
			}
			else if (contentsLower == "area") {
				response += std::to_string(a_effect->GetArea());
			}
			else if (contentsLower.starts_with("global=")) {
				auto globalEDID = contents.substr(7);
				const auto global = RE::TESForm::LookupByEditorID<RE::TESGlobal>(globalEDID);
				if (global) {
					response += std::to_string(global->value);
				}
			}
			else {
				response += contents;
			}
			response += closingTag;
			searchStart = matches.suffix().first;
		}
		response += std::string(searchStart, originalDescription.cend());
		return response;
	}
}