#pragma once

namespace EffectEvaluator
{
	bool ShouldSkipCondition(RE::TESConditionItem* a_condition,
		RE::PlayerCharacter* a_player,
		bool a_evaluateAll) {
		using funcID = RE::FUNCTION_DATA::FunctionID;
		if (!a_evaluateAll) {
			if (!(a_condition->data.object.any(RE::CONDITIONITEMOBJECT::kTarget))
				&& a_condition->data.runOnRef.get().get() != a_player->AsReference()) {
				return true;
			}
		}

		switch (a_condition->data.functionData.function.get()) {
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
			return false;
			break;
		default:
			break;
		}
		return true;
	}

	bool ShouldCheckAllConditions(RE::EffectSetting* a_effect) {
		const auto delivery = a_effect->data.delivery;
		const auto archetype = a_effect->GetArchetype();

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
				return false;
				break;
			default:
				break;
			}
		}
		return true;
	}

	bool AreConditionsValid(RE::Effect* a_effect, RE::ConditionCheckParams& a_params, RE::PlayerCharacter* a_player, bool a_checkAll) {
		auto effectCondHead = a_effect->conditions.head;
		bool matchedOR = false;
		while (effectCondHead) {
			if (ShouldSkipCondition(effectCondHead, a_player, a_checkAll)) {
				effectCondHead = effectCondHead->next;
				continue;
			}

			if (effectCondHead->data.flags.isOR) {
				if (!effectCondHead->IsTrue(a_params)) {
					effectCondHead = effectCondHead->next;
					continue;
				}
				matchedOR = true;
			}
			else {
				if (!matchedOR && !effectCondHead->IsTrue(a_params)) {
					return false;
				}
				matchedOR = false;
			}
			effectCondHead = effectCondHead->next;
		}

		auto baseCondHead = a_effect->baseEffect->conditions.head;
		matchedOR = false;
		while (baseCondHead) {
			if (ShouldSkipCondition(baseCondHead, a_player, a_checkAll)) {
				baseCondHead = baseCondHead->next;
				continue;
			}

			if (baseCondHead->data.flags.isOR) {
				if (!baseCondHead->IsTrue(a_params)) {
					baseCondHead = baseCondHead->next;
					continue;
				}
				matchedOR = true;
			}
			else {
				if (!matchedOR && !baseCondHead->IsTrue(a_params)) {
					return false;
				}
				matchedOR = false;
			}
			baseCondHead = baseCondHead->next;
		}

		return true;
	}

	RE::EffectSetting* GetMostEffectValidEffect(RE::MagicItem* a_this) {
		auto* argCostliest = a_this->GetCostliestEffectItem();
		auto* costliestBase = argCostliest ? argCostliest->baseEffect : nullptr;
		if (!costliestBase) {
			return nullptr;
		}

		RE::EffectSetting* response = nullptr;
		const auto player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return a_this->GetCostliestEffectItem()->baseEffect;
		}

		const auto& effects = a_this->effects;
		float costliest = -1.0f;
		auto params = RE::ConditionCheckParams(player, player);
		for (const auto effect : effects) {
			if (!effect || !effect->baseEffect || effect->cost < costliest) {
				continue;
			}

			const auto baseEffect = effect->baseEffect;
			if (!baseEffect->data.projectileBase) {
				continue;
			}

			if (effect->conditions.IsTrue(player, player) && baseEffect->conditions.IsTrue(player, player)) {
				response = baseEffect;
			}
			else {
				bool shouldMatchAllConditions = ShouldCheckAllConditions(baseEffect);
				if (AreConditionsValid(effect, params, player, shouldMatchAllConditions)) {
					response = baseEffect;
					costliest = effect->cost;
				}
			}
		}
		return response;
	}
}