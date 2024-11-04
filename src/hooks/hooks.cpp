#include "Hooks/hooks.h"

#include "conditions/conditions.h"
#include "utilities/utilities.h"

namespace Hooks {
	void Install()
	{
		SpellItemDescription::Install();
		logger::info("Wrote menu description hook.");
	}

	void SpellItemDescription::thunk(RE::ItemCard* a1, RE::SpellItem* a2, RE::BSString& a_out)
	{
		func(a1, a2, a_out);

		const auto player = RE::PlayerCharacter::GetSingleton();
		std::string openingTag = "<font face=\'$EverywhereMediumFont\' size=\'20\' color=\'#FFFFFF\'>";
		std::string closingTag = "</font>";
		std::string newDescription = "";

		for (const auto effect : a2->effects) {
			if (const auto baseEffect = effect->baseEffect;
				effect->conditions.IsTrue(player, nullptr) &&
				!baseEffect->data.flags.any(RE::EffectSetting::EffectSettingData::Flag::kHideInUI)) {
				std::string replaceMe = baseEffect->magicItemDescription.c_str();
				std::string mag = std::format("{}{}{}", openingTag, static_cast<uint32_t>(effect->GetMagnitude()), closingTag);
				std::string dur = std::format("{}{}{}", openingTag, static_cast<uint32_t>(effect->GetDuration()), closingTag);

				Utilities::String::replace_all(replaceMe, "<mag>", mag);
				Utilities::String::replace_all(replaceMe, "<dur>", dur);

				newDescription += replaceMe;
			}
		}

		a_out = newDescription;
	}
}