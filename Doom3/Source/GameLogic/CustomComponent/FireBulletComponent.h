#pragma once


#include <Doom_Core.h>

#include "FireBulletComponent.reflection.h"
namespace dooms
{
	class BulletComponent;

	class DOOM_API D_CLASS FireBulletComponent : public PlainComponent
	{
		GENERATE_BODY()
		
		

	public:

		BulletComponent* mBullet = nullptr;

		void InitComponent() override;
		void UpdateComponent() override;
	};
}

