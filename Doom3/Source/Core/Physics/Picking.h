#pragma once

#include "../Core.h"

#include <Physics/Collider/Ray.h>

namespace doom
{
	class GameCore;
	class ColliderComponent;
	namespace physics
	{
		class Collider;
		class DOOM_API Picking : public DObject, public ISingleton<Picking>
		{
		private:


		public:

			doom::physics::Ray GetCurrentCursorPointWorldRay();

		};
	}
}

