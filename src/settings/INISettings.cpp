#include "INISettings.h"

#include <SimpleIni.h>

namespace Settings::INI
{
	void Read()
	{
		INIHolder::GetSingleton()->Read();
	}

	void INIHolder::Read()
	{
		CSimpleIniA ini{};
		ini.SetUnicode();
		ini.LoadFile(fmt::format(R"(.\Data\SKSE\Plugins\{}.ini)", Plugin::NAME).c_str());

		if (ini.KeyExists("General", "bInstallProjectilePatch")) {
			this->projectilePatch = ini.GetBoolValue("General", "bInstallProjectilePatch", false);
		}
		else {
			this->projectilePatch = false;
		}

		if (ini.KeyExists("General", "bInstallImpactPatch")) {
			this->impactPatch = ini.GetBoolValue("General", "bInstallImpactPatch", false);
		}
		else {
			this->impactPatch = false;
		}
	}

	bool INIHolder::ShouldInstallProjectilePatch()
	{
		return this->projectilePatch;
	}

	bool INIHolder::ShouldInstallImpactPatch()
	{
		return this->impactPatch;
	}
}