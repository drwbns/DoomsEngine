#pragma once

#include "../Phycis_Core.h"

#include "RenderCollider.h"
#include <StaticContainer/StaticContainer.h>
#include "ColliderType.h"

namespace doom
{
	namespace physics
	{
		class Collider : public RenderCollider, public StaticContainer<Collider>
		{
		private:

			
			
		protected:

		public:

			bool bmIsCollideAtCurrentFrame{ false };
			void ClearCollision();

			Collider() {}
			~Collider() {}

			virtual ColliderType GetColliderType() const = 0;

			virtual void* data() = 0;
			virtual const void* data() const = 0;


		};

	

	}
}