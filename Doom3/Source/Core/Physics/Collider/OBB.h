#pragma once

#include "../Core.h"
#include "Collider.h"
#include <Matrix4x4.h>

#include "OBB.reflection.h"
namespace dooms
{
	namespace physics
	{
		/// <summary>
		/// 
		/// </summary>
		class DOOM_API D_CLASS OBB : public Collider
		{
			GENERATE_BODY()

			FORCE_INLINE virtual void* data() final
			{
				return nullptr;
			}

			FORCE_INLINE virtual const void* data() const final
			{
				return nullptr;
			}
		};
	}
}