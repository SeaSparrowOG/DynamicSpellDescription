#include "JSONSettings.h"

#include "RE/misc.h"
#include "utilities/utilities.h"
#include "hooks/hooks.h"

namespace {
	union VOID_PARAM
	{
		char* c;
		std::int32_t i;
		float        f;
		RE::TESForm* ptr;
	};

	static std::vector<std::string> findJsonFiles()
	{
		static constexpr std::string_view directory = R"(Data/SKSE/Plugins/DynamicSpellDescription)";
		std::vector<std::string> jsonFilePaths;
		for (const auto& entry : std::filesystem::directory_iterator(directory)) {
			if (entry.is_regular_file() && entry.path().extension() == ".json") {
				jsonFilePaths.push_back(entry.path().string());
			}
		}

		std::sort(jsonFilePaths.begin(), jsonFilePaths.end());
		return jsonFilePaths;
	}

	static bool AppendPerkConditionsToEffect(const Json::Value& a_perkConditions, RE::EffectSetting* a_baseEffect, bool inverted) {
		using PARAM_TYPE = RE::SCRIPT_PARAM_TYPE;
		using PARAMS = std::pair<std::optional<PARAM_TYPE>, std::optional<PARAM_TYPE>>;

		if (!a_perkConditions.isArray()) {
			logger::warn("  >Perks field exists, but is not an array. Entry will be skipped.");
			return false;
		}
		std::vector<RE::BGSPerk*> requiredPerks{};

		for (const auto& entryPerk : a_perkConditions) {
			if (!entryPerk.isString()) {
				logger::warn("  >Found non-string element in Perk array. Entry will be ignored.");
				return false;
			}

			const auto foundPerk = Utilities::Forms::GetFormFromString<RE::BGSPerk>(entryPerk.asString());
			if (!foundPerk) {
				logger::warn("  >Failed to resolve {}. Entry will be ignored.", entryPerk.asString());
				return false;
			}
			requiredPerks.push_back(foundPerk);
		}

		const auto player = RE::PlayerCharacter::GetSingleton();
		assert(player);
		if (!player) {
			logger::critical("PLAYER SINGLETON NOT FOUND - YOU WILL LIKELY CRASH!");
			return false;
		}

		for (auto perk : requiredPerks) {
			auto newConditionItem = new RE::TESConditionItem();

			RE::CONDITION_ITEM_DATA condData{};

			const auto ref = player->AsReference();
			condData.runOnRef = ref->CreateRefHandle();
			condData.object = RE::CONDITIONITEMOBJECT::kSelf;

			PARAMS paramPair = { PARAM_TYPE::kPerk, PARAM_TYPE::kInt };
			auto& [param1Type, param2Type] = paramPair;
			if (param2Type) {
				VOID_PARAM param{};
				param.i = inverted ? (int32_t)0 : (int32_t)1;
				condData.functionData.params[1] = std::bit_cast<void*>(param);
			}
			if (param1Type) {
				VOID_PARAM param{};
				param.ptr = perk;
				condData.functionData.params[0] = std::bit_cast<void*>(param);
			}

			condData.flags.opCode = RE::CONDITION_ITEM_DATA::OpCode::kEqualTo;
			condData.functionData.function = RE::FUNCTION_DATA::FunctionID::kHasPerk;
			condData.comparisonValue.f = inverted ? 0.0f : 1.0f;
			condData.flags.isOR = false;

			newConditionItem->data = condData;
			newConditionItem->next = nullptr;

			auto* tail = a_baseEffect->conditions.head;
			while (tail && tail->next) {
				tail = tail->next;
			}

			if (tail) {
				tail->next = newConditionItem;
			}
			else {
				a_baseEffect->conditions.head = newConditionItem;
			}
			newConditionItem->next = nullptr;
		}
		return true;
	}

	static void AppendEffectToSpell(const Json::Value& a_entry, RE::SpellItem* a_target, std::string a_newDescription) {
		const auto frontPointer = a_target->effects.front();
		const auto frontEffect = frontPointer ? frontPointer->baseEffect : nullptr;
		if (!frontEffect) {
			logger::critical("Spell {} has no effects! You WILL crash later.", a_target->GetName());
			return;
		}

		auto* factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::EffectSetting>();
		auto newBaseEffect = factory->Create();
		if (!newBaseEffect) {
			logger::error("  >Failed to create new base effect.");
			return;
		}

		auto newEffect = new RE::Effect();
		if (!newEffect) {
			logger::error("  >Failed to create new effect.");
			return;
		}

		const auto& perks = a_entry["perks"];
		if (perks) {
			if (!AppendPerkConditionsToEffect(perks, newBaseEffect, false)) {
				return;
			}
		}

		const auto& reversePerks = a_entry["!perks"];
		if (reversePerks) {
			if (!AppendPerkConditionsToEffect(reversePerks, newBaseEffect, true)) {
				return;
			}
		}

		newBaseEffect->data.archetype = RE::EffectSetting::Archetype::kScript;
		newBaseEffect->data.delivery = frontEffect->data.delivery;
		newBaseEffect->data.castingType = frontEffect->data.castingType;

		newEffect->baseEffect = newBaseEffect;
		newEffect->cost = 0.0f;
		newEffect->effectItem.area = 0;
		newEffect->effectItem.duration = 0;
		newEffect->effectItem.magnitude = 0.0f;

		a_target->effects.push_back(newEffect);
		Hooks::SpellItemDescription::GetSingleton()->RegisterNewEffectDescription(newBaseEffect, a_newDescription);
	}

	static void ProcessEffect(const Json::Value& a_entry) {
		const auto& effect = a_entry["effect"];
		const auto& description = a_entry["description"];
		if (!effect || !description) {
			logger::warn("  >Missing effect and/or description field. Entry will be skipped.");
			return;
		}
		if (!effect.isString() || !description.isString()) {
			logger::warn("  >Effect and/or description fields are not strings. Entry will be skipped.");
			return;
		}

		auto descriptionField = description.asString();
		const auto effectField = effect.asString();
		if (descriptionField.empty() || effectField.empty()) {
			logger::warn("  >Failed to parse description and/or effect/description fields. Entry will be skipped.");
			return;
		}

		auto effectForm = Utilities::Forms::GetFormFromString<RE::EffectSetting>(effectField);
		if (!effectForm) {
			logger::warn("  >Failed to resolve effect form for {}. Entry will be skipped.", effectField);
			return;
		}

		effectForm->magicItemDescription = RE::BSFixedString(descriptionField);
		effectForm->data.flags.reset(RE::EffectSetting::EffectSettingData::Flag::kHideInUI);
	}

	static void ProcessEntry(const Json::Value& a_entry) {
		const auto& spell = a_entry["spell"];
		const auto& description = a_entry["description"];
		if (!spell || !description) {
			logger::warn("  >Missing spell and/or description field! Entry will be skipped.");
			return;
		}
		if (!spell.isString() || !description.isString()) {
			logger::warn("  >Spell and/or description fields are not strings. Entry will be skipped.");
			return;
		}

		auto descriptionField = description.asString();
		const auto spellField = spell.asString();
		if (descriptionField.empty() || spellField.empty()) {
			logger::warn("  >Failed to parse description and/or spell fields. Entry will be skipped.");
			return;
		}

		const auto spellForm = Utilities::Forms::GetFormFromString<RE::SpellItem>(spellField);
		if (!spellForm) {
			logger::warn("  >Failed to resolve spell form for {}. Entry will be skipped.", spellField);
			return;
		}

		const auto frontEffect = spellForm->effects.front() && spellForm->effects.front()->baseEffect ? spellForm->effects.front()->baseEffect : nullptr;
		if (!frontEffect) {
			logger::warn("  >Spell {} is incorrectly created and will be ignored.", spellField);
			return;
		}
		AppendEffectToSpell(a_entry, spellForm, descriptionField);
	}

	static void ProcessDynamicEffect(const Json::Value& a_entry, std::vector<Assignment>& a_assignments) {
		const auto& descriptionField = a_entry["description"];
		if (!descriptionField || !descriptionField.isString()) {
			logger::warn("Dynamic Effect definition lacking description field, or description field is not a string. Entry will be skipped.");
			return;
		}

		std::vector<RE::BGSKeyword*> allowedPerks{};
		std::vector<RE::BGSKeyword*> disallowedPerks{};
		const auto& keywordsField = a_entry["keywords"];
		const auto& reversedKeywordsField = a_entry["!keywords"];
		if (keywordsField) {
			if (!keywordsField.isArray()) {
				logger::warn("Dynamic Effect definition has perks field, but it is not an array. Entry will be skipped.");
				return;
			}

			for (const auto& entry : keywordsField) {
				if (!entry.isString()) {
					logger::warn("Invalid keyword (not a string) in perks field. Entry will be skipped.");
					return;
				}

				auto* perk = RE::TESForm::LookupByEditorID<RE::BGSKeyword>(entry.asString());
				if (!perk) {
					logger::warn("Failed to resolve {} in perks. Entry will be skipped.", entry.asString());
					return;
				}
				allowedPerks.push_back(perk);
			}
		}

		if (reversedKeywordsField) {
			if (!reversedKeywordsField.isArray()) {
				logger::warn("Dynamic Effect definition has !perks field, but it is not an array. Entry will be skipped.");
				return;
			}

			for (const auto& entry : reversedKeywordsField) {
				if (!entry.isString()) {
					logger::warn("Invalid keyword (not a string) in !perks field. Entry will be skipped.");
					return;
				}

				auto* form = RE::TESForm::LookupByEditorID<RE::BGSKeyword>(entry.asString());
				if (!form) {
					logger::warn("Failed to resolve {} in !perks. Entry will be skipped.", entry.asString());
					return;
				}
				disallowedPerks.push_back(form);
			}
		}

		std::vector<std::unique_ptr<AssignmentRule>> newRules{};
		if (!allowedPerks.empty()) {
			auto newRule = AssignmentKeywordRule(allowedPerks, false);
			auto newRuleUnique = std::make_unique<AssignmentKeywordRule>(newRule);
			if (!newRuleUnique) {
				logger::critical("Failed unexpectedly while making a unique rule>");
				return;
			}
			newRules.push_back(std::move(newRuleUnique));
		}

		if (!disallowedPerks.empty()) {
			auto newRule = AssignmentKeywordRule(disallowedPerks, true);
			auto newRuleUnique = std::make_unique<AssignmentKeywordRule>(newRule);
			if (!newRuleUnique) {
				logger::critical("Failed unexpectedly while making a unique rule>");
				return;
			}
			newRules.push_back(std::move(newRuleUnique));
		}

		auto newAssignmentRule = Assignment(newRules, a_entry);
		a_assignments.push_back(std::move(newAssignmentRule));
	}

	void Assignment::AttemptAssign(RE::SpellItem* a_spell)
	{
		for (const auto& rule : rules) {
			if (!rule->IsValid(a_spell)) {
				return;
			}
		}
		AppendEffectToSpell(effectDefinition, a_spell, newDescription);
	}

	Assignment::Assignment(std::vector<std::unique_ptr<AssignmentRule>>& a_rules, Json::Value a_entry)
	{
		rules = std::move(a_rules);
		newDescription = a_entry["description"].asString();
		effectDefinition = a_entry;
	}

	AssignmentKeywordRule::AssignmentKeywordRule(std::vector<RE::BGSKeyword*>& a_keywords, bool a_inverted)
	{
		keywords = std::move(a_keywords);
		inverted = a_inverted;
	}

	bool AssignmentKeywordRule::IsValid(RE::SpellItem* a_spell)
	{
		const auto& spellEffects = a_spell->effects;
		for (const auto keyword : keywords) {
			bool found = false;
			for (auto it = spellEffects.begin(); !found && it != spellEffects.end(); ++it) {
				if (!(*it) || !(*it)->baseEffect) {
					continue;
				}

				found = (*it)->baseEffect->HasKeyword(keyword);
				if (inverted && found) {
					return false;
				}
			}
			if (!inverted && !found) {
				return false;
			}
		}
		return true;
	}
}


namespace Settings::JSON
{
	void Read()
	{
		std::vector<std::string> paths{};
		try {
			paths = findJsonFiles();
		}
		catch (const std::exception& e) {
			logger::warn("Caught {} while reading files.", e.what());
			return;
		}
		if (paths.empty()) {
			logger::info("No settings found");
			return;
		}

		std::vector<Assignment> assignments{};

		for (const auto& path : paths) {
			Json::Reader JSONReader;
			Json::Value JSONFile;
			try {
				std::ifstream rawJSON(path);
				JSONReader.parse(rawJSON, JSONFile);
			}
			catch (const Json::Exception& e) {
				logger::warn("Caught {} while reading files.", e.what());
				continue;
			}
			catch (const std::exception& e) {
				logger::error("Caught unhandled exception {} while reading files.", e.what());
				continue;
			}

			if (!JSONFile.isObject()) {
				logger::warn("Warning: <{}> is not an object. File will be ignored.", path);
				continue;
			}

			const auto& newEffects = JSONFile["newEffects"];
			if (newEffects && !newEffects.isArray()) {
				logger::warn("newEffects field is present, but is not an array.");
				continue;
			}
			else if (newEffects) {
				for (const auto& effectEntry : newEffects) {
					ProcessEntry(effectEntry);
				}
			}

			const auto& newDescriptions = JSONFile["newDescriptions"];
			if (newDescriptions && !newDescriptions.isArray()) {
				logger::warn("newDescriptions field is present, but is not an array.");
				continue;
			}
			else if (newDescriptions) {
				for (const auto& descriptionEntry : newDescriptions) {
					ProcessEffect(descriptionEntry);
				}
			}

			const auto& dynamicEffects = JSONFile["dynamicEffects"];
			if (dynamicEffects) {
				if (!dynamicEffects.isArray()) {
					logger::warn("dynamicEffects field is present, but is not an array.");
					continue;
				}
				for (const auto& entry : dynamicEffects) {
					ProcessDynamicEffect(entry, assignments);
				}
			}
		}

		if (!assignments.empty()) {
			const auto dataHandler = RE::TESDataHandler::GetSingleton();
			if (!dataHandler) {
				logger::critical("Failed to get the Data Handler. This will cause a crash later.");
				return;
			}

			const auto& spellArray = dataHandler->GetFormArray<RE::SpellItem>();
			for (auto* spell : spellArray) {
				if (!spell || spell->effects.empty()) {
					continue;
				}

				for (auto& assignment : assignments) {
					assignment.AttemptAssign(spell);
				}
			}
		}
	}
}