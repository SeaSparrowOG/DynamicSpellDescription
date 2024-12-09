#include "Hooks/hooks.h"

namespace Hooks {
	void Install()
	{
		SKSE::AllocTrampoline(14);
		SpellItemDescription::Install();
		logger::info("Wrote menu description hook.");
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
		using delivery = RE::MagicSystem::Delivery;
		using conditionObject = RE::CONDITIONITEMOBJECT;
		_getSpellDescription(a1, a2, a_out);

		const auto player = RE::PlayerCharacter::GetSingleton();
		const auto singleton = SpellItemDescription::GetSingleton();
		assert(player && singleton);
		if (!player || !singleton) {
			logger::error("Failed to get the player or SpellItemDescirptionSingleton. This will likely cause a crash later.");
			return;
		}

		std::string newDescription = "";
		for (const auto effect : a2->effects) {
			if (effect->baseEffect
				&& singleton->specialEffects.contains(effect->baseEffect)) {
				const auto baseEffect = effect->baseEffect;
				if (baseEffect->data.delivery != delivery::kSelf) {
					if (!singleton->ShouldIgnoreSelfCondtions(baseEffect->conditions)
						&& !effect->conditions.IsTrue(player, player)) {
						continue;
					}
					if (!singleton->ShouldOtherTargetEffectApply(baseEffect)) {
						continue;
					}
				}
				else {
					if (!effect->conditions.IsTrue(player, player)
						&& !singleton->ShouldIgnoreSelfCondtions(effect->conditions)) {
						continue;
					}
					else if (!singleton->ShouldOtherTargetEffectApply(baseEffect)) {
						continue;
					}
				}
				newDescription += singleton->specialEffects[baseEffect];
			}
			else if (const auto baseEffect = effect->baseEffect; 
				baseEffect 
				&& !baseEffect->data.flags.any(RE::EffectSetting::EffectSettingData::Flag::kHideInUI)
				&& !baseEffect->magicItemDescription.empty()) {

				if (baseEffect->data.delivery != delivery::kSelf) {
					if (!singleton->ShouldIgnoreSelfCondtions(baseEffect->conditions)
						&& !effect->conditions.IsTrue(player, player)) {
						continue;
					}
					if (!singleton->ShouldOtherTargetEffectApply(baseEffect)) {
						continue;
					}
				}
				else {
					if (!effect->conditions.IsTrue(player, player)
						&& !singleton->ShouldIgnoreSelfCondtions(effect->conditions)) {
						continue;
					}
					else if (!singleton->ShouldOtherTargetEffectApply(baseEffect)) {
						continue;
					}
				}
				newDescription += singleton->GetFormattedEffectDescription(baseEffect, effect);
			}
		}

		if (newDescription.empty()) {
			return;
		}
		a_out = newDescription;
	}

	bool SpellItemDescription::ShouldIgnoreSelfCondtions(const RE::TESCondition& a_condition)
	{
		using funcID = RE::FUNCTION_DATA::FunctionID;
		if (!a_condition.head) {
			return true;
		}

		auto head = a_condition.head;
		while (head) {
			if (head->data.functionData.function.any(
				funcID::kGetBaseActorValue,
				funcID::kGetIsPlayableRace,
				funcID::kGetIsRace,
				funcID::kGetIsSex,
				funcID::kGetLevel,
				funcID::kGetQuestCompleted,
				funcID::kGetQuestRunning,
				funcID::kGetStageDone,
				funcID::kGetStage,
				funcID::kGetVMQuestVariable,
				funcID::kGetVMScriptVariable,
				funcID::kHasPerk
			)) {
				return false;
			}
			head = head->next;
		}
		return true;
	}

	bool SpellItemDescription::ShouldSelfTargetEffectApply(RE::EffectSetting* a_baseEffect)
	{
		auto head = a_baseEffect->conditions.head;
		if (!head) {
			return true;
		}

		const auto player = RE::PlayerCharacter::GetSingleton();
		if (a_baseEffect->conditions.IsTrue(player, player)) {
			return true;
		}
		else if (ShouldIgnoreSelfCondtions(a_baseEffect->conditions)) {
			return true;
		}
		return false;
	}

	bool SpellItemDescription::ShouldOtherTargetEffectApply(RE::EffectSetting* a_baseEffect)
	{
		using funcID = RE::FUNCTION_DATA::FunctionID;
		auto head = a_baseEffect->conditions.head;
		if (!head) {
			return true;
		}

		const auto player = RE::PlayerCharacter::GetSingleton();
		if (a_baseEffect->conditions.IsTrue(player, player)) {
			return true;
		}
		else if (ShouldIgnoreSelfCondtions(a_baseEffect->conditions)) {
			return true;
		}

		auto parametersToCheck = RE::ConditionCheckParams(player, player);
		while (head) {
			if (!head->data.functionData.function.any(
				funcID::kGetBaseActorValue,
				funcID::kGetIsPlayableRace,
				funcID::kGetIsRace,
				funcID::kGetIsSex,
				funcID::kGetLevel,
				funcID::kGetQuestCompleted,
				funcID::kGetQuestRunning,
				funcID::kGetStageDone,
				funcID::kGetStage,
				funcID::kGetVMQuestVariable,
				funcID::kGetVMScriptVariable,
				funcID::kHasPerk
			)) {
				head = head->next;
				continue;
			}
			if (head->IsTrue(parametersToCheck)) {
				head = head->next;
				continue;
			}
			if (head->data.flags.swapTarget
				&& head->data.object.any(RE::CONDITIONITEMOBJECT::kTarget)) {
				head = head->next;
				continue;
			}
			if (head->data.runOnRef.get().get() != player->AsReference()) {
				head = head->next;
				continue;
			}
			return false;
		}
		return true;
	}

	std::string SpellItemDescription::GetFormattedEffectDescription(RE::EffectSetting* a_baseEffect, RE::Effect* a_effect)
	{
		std::string originalDescription = a_baseEffect->magicItemDescription.c_str();
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