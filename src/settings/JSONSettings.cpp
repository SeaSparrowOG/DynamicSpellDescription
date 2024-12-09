#include "JSONSettings.h"

#include "RE/misc.h"
#include "utilities/utilities.h"


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

	static void ProcessEntry(const Json::Value& a_entry) {
		using PARAM_TYPE = RE::SCRIPT_PARAM_TYPE;
		using PARAMS = std::pair<std::optional<PARAM_TYPE>, std::optional<PARAM_TYPE>>;

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

		const auto descriptionField = description.asString();
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

		auto newBaseEffect = RE::CreateEffectSetting();
		if (!newBaseEffect) {
			logger::error("  >Failed to create new base effect for spell {}. While not fatal, this could indicate a bug or instability.", spellField);
			return;
		}
		
		auto newEffect = new RE::Effect();
		if (!newEffect) {
			logger::error("  >Failed to create new effect for {}.", spellField);
			return;
		}

		const auto& perks = a_entry["perks"];
		if (perks) {
			if (!perks.isArray()) {
				logger::warn("  >Perks field exists, but is not an array. Entry will be skipped.");
				return;
			}
			std::vector<RE::BGSPerk*> requiredPerks{};

			for (const auto& entryPerk : perks) {
				if (!entryPerk.isString()) {
					logger::warn("  >Found non-string element in Perk array for {}. Only that perk will be ignored.", spellField);
					continue;
				}

				const auto foundPerk = Utilities::Forms::GetFormFromString<RE::BGSPerk>(entryPerk.asString());
				if (!foundPerk) {
					logger::warn("  >Failed to resolve {} in {}. Only that perk will be ignored.", entryPerk.asString(), spellField);
					continue;
				}
				requiredPerks.push_back(foundPerk);
			}

			const auto player = RE::PlayerCharacter::GetSingleton();
			assert(player);
			if (!player) {
				logger::critical("PLAYER SINGLETON NOT FOUND - YOU WILL LIKELY CRASH!");
				return;
			}
			const auto playerHandle = player->GetHandle();
			auto newCondition = RE::TESCondition();
			for (auto perk : requiredPerks) {
				auto newConditionItem = new RE::TESConditionItem();

				newConditionItem->data.comparisonValue.f = 1.0f;
				newConditionItem->data.flags.isOR = false;
				newConditionItem->data.flags.global = false;
				newConditionItem->data.flags.opCode = RE::CONDITION_ITEM_DATA::OpCode::kEqualTo;
				newConditionItem->data.flags.swapTarget = false;
				newConditionItem->data.flags.usePackData = false;
				newConditionItem->data.flags.usesAliases = false;

				newConditionItem->data.object.set(RE::CONDITIONITEMOBJECT::kRef);
				newConditionItem->data.runOnRef = playerHandle;

				newConditionItem->data.functionData.function.set(RE::FUNCTION_DATA::FunctionID::kHasPerk);

				PARAMS paramPair = { PARAM_TYPE::kPerk, PARAM_TYPE::kInt };
				auto& [param1Type, param2Type] = paramPair;
				// param1
				if (param2Type) {
					VOID_PARAM param{};
					param.i = (int32_t)1;
					newConditionItem->data.functionData.params[1] = std::bit_cast<void*>(param);
				}
				// param2
				if (param1Type) {
					VOID_PARAM param{};
					param.ptr = perk->As<RE::TESForm>();
					newConditionItem->data.functionData.params[0] = std::bit_cast<void*>(param);
				}
				newConditionItem->data.functionData.params;
				if (!newCondition.head) {
					newCondition.head = newConditionItem;
					newConditionItem->next = nullptr;
				}
				else {
					auto head = newCondition.head;
					while (head && head->next) {
						head = head->next;
					}

					head->next = newConditionItem;
					newConditionItem->next = nullptr;
				}
			}

			newBaseEffect->conditions = newCondition;
		}

		newBaseEffect->data.archetype = RE::EffectSetting::Archetype::kScript;
		newBaseEffect->data.delivery = frontEffect->data.delivery;
		newBaseEffect->data.castingType = frontEffect->data.castingType;
		newBaseEffect->magicItemDescription = "newDescription";

		newEffect->baseEffect = newBaseEffect;
		newEffect->cost = 0.0f;
		newEffect->effectItem.area = 0;
		newEffect->effectItem.duration = 0;
		newEffect->effectItem.magnitude = 0.0f;
		spellForm->effects.push_back(newEffect);
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
			if (!newEffects) {
				logger::warn("Missing newEffects field. Check spelling and capitalization!");
				continue;
			}
			if (!newEffects.isArray()) {
				logger::warn("newEffects field is present, but is not an array.");
				continue;
			}

			for (const auto& effectEntry : newEffects) {
				ProcessEntry(effectEntry);
			}
		}
	}
}