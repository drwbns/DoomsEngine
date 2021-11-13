#pragma once

#include <Doom_Core.h>

#include "BoxAdder.reflection.h"
namespace dooms
{
	class DOOM_API D_CLASS BoxAdder : public PlainComponent
	{
		GENERATE_BODY()
		
		

	protected:
		void InitComponent() override;


		void UpdateComponent() override;


		void OnEndOfFrame_Component() override;

	};
}

