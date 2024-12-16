#pragma once

#include "utilities/utilities.h"

namespace Settings
{
	namespace INI
	{
		void Read();

		class INIHolder : public Utilities::Singleton::ISingleton<INIHolder>
		{
		public:
			void Read();
			bool ShouldInstallProjectilePatch();
			bool ShouldInstallImpactPatch();

		private:
			bool projectilePatch{ false };
			bool impactPatch{ false };
		};
	}
}