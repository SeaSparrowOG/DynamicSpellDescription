#include "settings/settings.h"

#include "conditions/conditions.h"
#include "utilities/utilities.h"

namespace {
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
}

namespace Settings
{
	void Read() {
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
			const auto& rules = JSONFile["rules"];
			if (!rules || !rules.isArray()) {
				logger::warn("Warning: <{}> is not a valid configuration file, rules is missing or is not array.", path);
				continue;
			}

			for (const auto& rule : rules) {
				if (!rule.isObject()) {
					logger::warn("Warning: Rule violation in <{}>, rule is not an object.", path);
					continue;
				}
				const auto& description = rule["description"];
				if (!description || description.isString()) {
					logger::warn("Warning: description in rule is not a string (File: <{}>)", path);
					continue;
				}
				const auto& spell = rule["spell"];
				if (!spell || !spell.isString()) {
					logger::warn("Warning: missing spell in rule, or spell is not a string (File: <{}>).", path);
					continue;
				}

				RE::SpellItem* spellTarget = Utilities::Forms::GetFormFromString<RE::SpellItem>(spell.asString());
				if (!spellTarget) {
					logger::warn("Warning: Spell [{}] specified in file <{}> not found.", spell.asString(), path);
					continue;
				}

				std::vector<RE::BGSPerk*> requiredANDPerksVector{};
				std::vector<RE::BGSPerk*> requiredORPerksVector{};
				std::vector<RE::BGSPerk*> excludedANDPerksVector{};
				std::vector<RE::BGSPerk*> excludedORPerksVector{};
				std::string descriptionAddition = description.asString();

				const auto& requiredANDPerks = rule["requiredANDPerks"];
				if (requiredANDPerks) {
					if (!requiredANDPerks.isArray()) {
						continue;
					}

					for (const auto& rawPerk : requiredANDPerks) {
						if (!rawPerk.isString()) {
							continue;
						}
						const auto perk = Utilities::Forms::GetFormFromString<RE::BGSPerk>(rawPerk.asString());
						if (!perk) {
							continue;
						}

						requiredANDPerksVector.push_back(std::move(perk));
					}
				}

				const auto& requiredORPerks = rule["requiredORPerks"];
				if (requiredORPerks) {
					if (!requiredORPerks.isArray()) {
						continue;
					}

					for (const auto& rawPerk : requiredORPerks) {
						if (!rawPerk.isString()) {
							continue;
						}
						const auto perk = Utilities::Forms::GetFormFromString<RE::BGSPerk>(rawPerk.asString());
						if (!perk) {
							continue;
						}

						requiredORPerksVector.push_back(std::move(perk));
					}
				}

				const auto& excludedANDPerks = rule["excludedANDPerks"];
				if (excludedANDPerks) {
					if (!excludedANDPerks.isArray()) {
						continue;
					}

					for (const auto& rawPerk : excludedANDPerks) {
						if (!rawPerk.isString()) {
							continue;
						}
						const auto perk = Utilities::Forms::GetFormFromString<RE::BGSPerk>(rawPerk.asString());
						if (!perk) {
							continue;
						}

						excludedANDPerksVector.push_back(std::move(perk));
					}
				}

				const auto& excludedORPerks = rule["excludedORPerks"];
				if (excludedORPerks) {
					if (!excludedORPerks.isArray()) {
						continue;
					}

					for (const auto& rawPerk : excludedORPerks) {
						if (!rawPerk.isString()) {
							continue;
						}
						const auto perk = Utilities::Forms::GetFormFromString<RE::BGSPerk>(rawPerk.asString());
						if (!perk) {
							continue;
						}

						excludedORPerksVector.push_back(std::move(perk));
					}
				}


				Conditions::Cache::ComplexCondition newConditions{};
				newConditions.excludedANDPerks = excludedANDPerksVector;
				newConditions.excludedORPerks = excludedORPerksVector;
				newConditions.requiredANDPerks = requiredANDPerksVector;
				newConditions.requiredORPerks = requiredORPerksVector;

				Conditions::Cache::GetSingleton()->AddCondition(spellTarget, std::move(newConditions));
			}			
		}
	}
}