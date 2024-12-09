#pragma once

#include "RE/Offset.h"

namespace RE
{
	inline EffectSetting* CreateEffectSetting() {
		using func_t = decltype(&CreateEffectSetting);
		static REL::Relocation<func_t> func{ Offset::EffectSetting::factory };
		return func();
	}
}