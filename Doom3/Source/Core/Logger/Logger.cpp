#include "Logger.h"

#include <cstdarg>
#include <string>
#include <cstdio>

#include "../EngineConfigurationData/ConfigData.h"
#include "LoggerSetting.h"

namespace dooms
{
	namespace logger
	{
		



	}
}


void dooms::logger::InitLogger()
{
	MIN_DEBUG_LEVEL = static_cast<eLogType>(::dooms::ConfigData::GetSingleton()->GetConfigData().GetValue<INT32>("SYSTEM", "MIN_DEBUG_LEVEL"));
	MAX_DEBUG_LEVEL = static_cast<eLogType>(::dooms::ConfigData::GetSingleton()->GetConfigData().GetValue<INT32>("SYSTEM", "MAX_DEBUG_LEVEL"));
}

void dooms::logger::StopIfError(eLogType logType)
{
	if (BREAK_WHEN_ERROR && logType == eLogType::D_ERROR)
	{
		breakpoint();
	}
}


void dooms::logger::breakpoint()
{
	std::fflush(stdout);
	__debugbreak();
}

bool dooms::logger::CheckLogAcceptable(const eLogType logType)
{
	return logType == eLogType::D_ALWAYS || (logType >= MIN_DEBUG_LEVEL && logType <= MAX_DEBUG_LEVEL);
}

const char* dooms::logger::LogTypeStr(const eLogType logType)
{
	switch (logType)
	{
	case eLogType::D_LOG:
		return "LOG";
		break;

	case eLogType::D_WARNING:
		return "WARNING";
		break;

	case eLogType::D_ERROR:
		return "ERROR";
		break;

	default:
		__assume(0);
	}

	return "ERROR";
}

