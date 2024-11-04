#include "Hooks/hooks.h"

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
	}
}