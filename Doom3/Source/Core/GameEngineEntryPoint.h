#pragma once

#include "Core.h"
#include "Game/GameCore.h"

#include <DObject/DObjectManager.h>
#include <OS/ErrorHandling.h>
#include <Graphics/GraphicsAPI/Manager/GraphicsAPIManager.h>

//void ExitGame();


#include <iostream>
#include <cstdio>

namespace dooms
{
	int GameEngineEntryPoint(int argc, char* argv[])
	{
		{
			{
				dooms::errorHandling::SetUnHandledExceptionHandling();

				dooms::GameCore gameCore{};

				D_START_PROFILING(Init_Game, dooms::profiler::eProfileLayers::CPU);
				gameCore.Init(argc, argv);
				D_END_PROFILING(Init_Game);

				while (gameCore.Tick())
				{
				}
				gameCore.CleanUp();
				//window terminated
			}

			dooms::DObjectManager::DestroyAllDObjects(false); // for detecting memory leak

		}

		dooms::graphics::GraphicsAPIManager::DeInitialize();

		D_ASSERT_LOG(dooms::DObjectManager::GetDObjectCount() == 0, "Never use DObject as static, global variable");
		std::cout << "\Remained DObject Count : " << dooms::DObjectManager::GetDObjectCount() << std::endl;
		std::fflush(stdout);

		return 0;
	}

}



