#pragma once

namespace Hooks {
	void Install();

	struct SpellItemDescription {
		static bool Install() {
			REL::Relocation<std::uintptr_t> target{ REL::ID(51898), 0x5DC };
			stl::write_thunk_call<SpellItemDescription>(target.address());
			return true;
		}

		static void thunk(RE::ItemCard* a1, RE::SpellItem* a2, RE::BSString& a_out);
		inline static REL::Relocation<decltype(SpellItemDescription::thunk)> func;
	};
}