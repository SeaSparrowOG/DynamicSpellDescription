#pragma once

#include "utilities/utilities.h"

namespace Hooks {
	void Install();

	class SpellItemDescription : public Utilities::Singleton::ISingleton<SpellItemDescription> 
	{
	public:
		static void Install() {
			REL::Relocation<std::uintptr_t> target{ REL::ID(51898), 0x5DC };

			auto& trampoline = SKSE::GetTrampoline();
			_getSpellDescription = trampoline.write_call<5>(target.address(), GetSpellDescription);
		}

		void RegisterNewEffectDescription(RE::EffectSetting* a_effect, std::string& a_newDescription);

	private:
		static void GetSpellDescription(RE::ItemCard* a1, RE::SpellItem* a2, RE::BSString& a_out);
		inline static REL::Relocation<decltype(SpellItemDescription::GetSpellDescription)> _getSpellDescription;

		bool ShouldIgnoreSelfCondtions(const RE::TESCondition& a_condition);
		bool ShouldSelfTargetEffectApply(RE::EffectSetting* a_baseEffect);
		bool ShouldOtherTargetEffectApply(RE::EffectSetting* a_baseEffect);
		std::string GetFormattedEffectDescription(RE::EffectSetting* a_baseEffect, RE::Effect* a_effect);
		
		std::unordered_map<RE::EffectSetting*, std::string> specialEffects{};

		std::string_view openingTag{ "<font face=\'$EverywhereMediumFont\' size=\'20\' color=\'#FFFFFF\'>" };
		std::string_view closingTag{ "</font>" };
		std::regex pattern{ R"(<(.*?)>)" };
	};
}