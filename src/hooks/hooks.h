#pragma once

#include "utilities/utilities.h"
#include "settings/INISettings.h"

namespace Hooks {
	void Install();

	class SpellItemDescription : public Utilities::Singleton::ISingleton<SpellItemDescription> 
	{
	public:
		static void Install() {
			REL::Relocation<std::uintptr_t> getSpellDescriptionTarget{ REL::ID(51898), 0x5DC };
			REL::Relocation<std::uintptr_t> getSpellTomeDescriptionTarget{ REL::ID(51897), 0xF5C };
			REL::Relocation<std::uintptr_t> getMagicWeaponDescriptionTarget{ REL::ID(51897), 0x1377 };

			REL::Relocation<std::uintptr_t> getProjectileTarget{ REL::ID(34450), 0x53 };
			REL::Relocation<std::uintptr_t> getImpactDataTarget{ REL::ID(44100), 0x228 };

			auto& trampoline = SKSE::GetTrampoline();

			if (Settings::INI::INIHolder::GetSingleton()->ShouldInstallProjectilePatch()) {
				_processProjectile = trampoline.write_call<5>(getProjectileTarget.address(), &ProcessProjectile);
			}

			if (Settings::INI::INIHolder::GetSingleton()->ShouldInstallImpactPatch()) {
				_processImpact = trampoline.write_call<5>(getImpactDataTarget.address(), &ProcessImpact);
			}

			_getSpellDescription = trampoline.write_call<5>(getSpellDescriptionTarget.address(), GetSpellDescription);
			_getSpellTomeDescription = trampoline.write_call<5>(getSpellTomeDescriptionTarget.address(), GetSpellTomeDescription);
			_getMagicWeaponDescription = trampoline.write_call<5>(getMagicWeaponDescriptionTarget.address(), GetMagicWeaponDescription);
		}

		void RegisterNewEffectDescription(RE::EffectSetting* a_effect, std::string& a_newDescription);

	private:
		static void GetSpellDescription(RE::ItemCard* a1, RE::SpellItem* a2, RE::BSString& a_out);
		static void GetSpellTomeDescription(RE::ItemCard* a1, RE::SpellItem* a2, RE::BSString& a_out);
		static void GetMagicWeaponDescription(RE::ItemCard* a1, RE::SpellItem* a2, RE::BSString& a_out);

		static RE::EffectSetting* ProcessProjectile(RE::MagicItem* a_this);
		static RE::EffectSetting* ProcessImpact(RE::MagicItem* a_this);

		inline static REL::Relocation<decltype(SpellItemDescription::GetSpellDescription)> _getSpellDescription;
		inline static REL::Relocation<decltype(SpellItemDescription::GetSpellTomeDescription)> _getSpellTomeDescription;
		inline static REL::Relocation<decltype(SpellItemDescription::GetMagicWeaponDescription)> _getMagicWeaponDescription;

		inline static REL::Relocation<decltype(SpellItemDescription::ProcessProjectile)> _processProjectile;
		inline static REL::Relocation<decltype(SpellItemDescription::ProcessImpact)> _processImpact;

		std::string GetFormattedEffectDescription(RE::Effect* a_effect);
		std::unordered_map<RE::EffectSetting*, std::string> specialEffects{};

		std::string_view openingTag{ "<font face=\'$EverywhereMediumFont\' size=\'20\' color=\'#FFFFFF\'>" };
		std::string_view closingTag{ "</font>" };
		std::regex pattern{ R"(<(.*?)>)" };
	};
}