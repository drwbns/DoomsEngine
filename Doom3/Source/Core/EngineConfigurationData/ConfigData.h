#pragma once
#include <Core.h>
#include "Misc/IniFile/SimpleIniParser.h"

#include <../Helper/Simple_SingleTon/Singleton.h>

namespace dooms
{
	class GameCore;
	class DOOM_API D_CLASS ConfigData : public DObject, public ISingleton<ConfigData>
	{
		friend class GameCore;

	private:

		const char* mConfigFilePath;
		IniData mConfigData;

	public:
		
		ConfigData();

		IniData& GetConfigData();
	};
}