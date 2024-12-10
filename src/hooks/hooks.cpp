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
			const auto baseEffect = effect ? effect->baseEffect : nullptr;
			if (!baseEffect) {
				continue;
			}
			if (baseEffect->data.flags.any(RE::EffectSetting::EffectSettingData::Flag::kHideInUI)) {
				continue;
			}
			
			if (singleton->IsEffectValid(effect)) {
				newDescription += singleton->GetFormattedEffectDescription(effect);
			}
		}

		if (newDescription.empty()) {
			return;
		}
		a_out = newDescription;
	}

	bool SpellItemDescription::IsEffectValid(RE::Effect* a_effect)
	{
		using funcID = RE::FUNCTION_DATA::FunctionID;
		const auto player = RE::PlayerCharacter::GetSingleton(); //Maybe pass that in as an argument?
		const auto baseEffect = a_effect->baseEffect;
		const auto archetype = baseEffect->data.archetype;
		const auto delivery = baseEffect->data.delivery;
		auto params = RE::ConditionCheckParams(player, player);
		auto condHead = a_effect->conditions.head;

		//OR into AND is different from AND into OR. This will somewhat
		//emulate it. I hope. I am not REing how Bethesda made it work.
		bool matchedOR = false;

		//The following archetypes apply only to the target. Things like Summon
		//affect the CASTER, and thus we cannot use delivery to determine if the
		//condition should be skipped.
		bool checkAllConditions = true;
		if (delivery != RE::MagicSystem::Delivery::kSelf) {
			switch (archetype) {
			case RE::EffectSetting::Archetype::kAbsorb:
			case RE::EffectSetting::Archetype::kBanish:
			case RE::EffectSetting::Archetype::kCalm:
			case RE::EffectSetting::Archetype::kConcussion:
			case RE::EffectSetting::Archetype::kDemoralize:
			case RE::EffectSetting::Archetype::kDisarm:
			case RE::EffectSetting::Archetype::kDualValueModifier:
			case RE::EffectSetting::Archetype::kEtherealize:
			case RE::EffectSetting::Archetype::kFrenzy:
			case RE::EffectSetting::Archetype::kGrabActor:
			case RE::EffectSetting::Archetype::kInvisibility:
			case RE::EffectSetting::Archetype::kLight:
			case RE::EffectSetting::Archetype::kLock:
			case RE::EffectSetting::Archetype::kOpen:
			case RE::EffectSetting::Archetype::kParalysis:
			case RE::EffectSetting::Archetype::kPeakValueModifier:
			case RE::EffectSetting::Archetype::kRally:
			case RE::EffectSetting::Archetype::kSoulTrap:
			case RE::EffectSetting::Archetype::kStagger:
			case RE::EffectSetting::Archetype::kTelekinesis:
			case RE::EffectSetting::Archetype::kTurnUndead:
			case RE::EffectSetting::Archetype::kValueAndParts:
			case RE::EffectSetting::Archetype::kValueModifier:
				checkAllConditions = false;
				break;
			default:
				break;
			}
		}

		while (condHead) {
			if (!checkAllConditions) {
				if (!(condHead->data.object.any(RE::CONDITIONITEMOBJECT::kTarget))
					&& condHead->data.runOnRef.get().get() != player->AsReference()) {
					condHead = condHead->next;
					continue;
				}
			}

			bool unneededOperation = true;
			switch (condHead->data.functionData.function.get()) {
			case funcID::kGetBaseActorValue:
			case funcID::kGetIsPlayableRace:
			case funcID::kGetIsRace:
			case funcID::kGetIsSex:
			case funcID::kGetLevel:
			case funcID::kGetQuestCompleted:
			case funcID::kGetQuestRunning:
			case funcID::kGetStageDone:
			case funcID::kGetStage:
			case funcID::kGetVMQuestVariable:
			case funcID::kGetVMScriptVariable:
			case funcID::kHasPerk:
				unneededOperation = false;
				break;
			default:
				break;
			}
			if (unneededOperation) {
				condHead = condHead->next;
				continue;
			}

			if (condHead->data.flags.isOR) {
				if (!condHead->IsTrue(params)) {
					condHead = condHead->next;
					continue;
				}
				matchedOR = true;
			}
			else {
				if (!matchedOR && !condHead->IsTrue(params)) {
					return false;
				}
				matchedOR = false;
			}
			condHead = condHead->next;
		}

		matchedOR = false;
		auto effectHead = baseEffect->conditions.head;
		while (effectHead) {
			if (!checkAllConditions) {
				if (!effectHead->data.object.any(RE::CONDITIONITEMOBJECT::kTarget)
					&& effectHead->data.runOnRef.get().get() != player->AsReference()) {
					effectHead = effectHead->next;
					continue;
				}
			}

			bool unneededOperation = true;
			switch (effectHead->data.functionData.function.get()) {
			case funcID::kGetBaseActorValue:
			case funcID::kGetIsPlayableRace:
			case funcID::kGetIsRace:
			case funcID::kGetIsSex:
			case funcID::kGetLevel:
			case funcID::kGetQuestCompleted:
			case funcID::kGetQuestRunning:
			case funcID::kGetStageDone:
			case funcID::kGetStage:
			case funcID::kGetVMQuestVariable:
			case funcID::kGetVMScriptVariable:
			case funcID::kHasPerk:
				unneededOperation = false;
				break;
			default:
				break;
			}
			if (unneededOperation) {
				effectHead = effectHead->next;
				continue;
			}
			
			if (effectHead->data.flags.isOR) {
				if (!effectHead->IsTrue(params)) {
					effectHead = effectHead->next;
					continue;
				}
				matchedOR = true;
			}
			else {
				if (!matchedOR && !effectHead->IsTrue(params)) {
					return false;
				}
				matchedOR = false;
			}
			effectHead = effectHead->next;
		}
		return true;
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