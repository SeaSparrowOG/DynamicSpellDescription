#pragma once

namespace {
	struct AssignmentRule
	{
	public:
		virtual bool IsValid(RE::SpellItem* a_spell) = 0;

		bool inverted;
	};

	struct AssignmentKeywordRule : public AssignmentRule
	{
	public:
		AssignmentKeywordRule(std::vector<RE::BGSKeyword*>& a_keywords, bool a_inverted);
		bool IsValid(RE::SpellItem* a_spell) override;
	private:
		std::vector<RE::BGSKeyword*> keywords;
	};

	struct Assignment
	{
	public:
		void AttemptAssign(RE::SpellItem* a_spell);

		Assignment(std::vector<std::unique_ptr<AssignmentRule>>& a_rules, Json::Value a_entry);
	private:
		std::vector<std::unique_ptr<AssignmentRule>> rules;
		std::string newDescription;
		Json::Value effectDefinition;
	};
}

namespace Settings
{
	namespace JSON
	{
		void Read();
	}
}