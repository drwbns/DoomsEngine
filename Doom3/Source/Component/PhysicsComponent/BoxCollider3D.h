#pragma once
#include "Collider3DComponent.h"
#include <Vector3.h>
#include <Collider/AABB.h>

#include "BoxCollider3D.reflection.h"

namespace dooms
{
	class DOOM_API D_CLASS BoxCollider3D : public Collider3DComponent
	{
		GENERATE_BODY()
		

		friend class physics::Physics_Server;
	private:

		/// <summary>
		/// world aabb
		/// </summary>
		physics::AABB3D mLocalAABB3D;
		physics::AABB3D mWorldAABB3D;

		math::Vector3 mHalfExtent;

		virtual void UpdateLocalCollider() final;
		virtual void UpdateWorldCollider() final;

		void AutoColliderSettingFromAABB3D(const physics::AABB3D& aabb3dFromMesh) final;
	



	public:		
		void SetFromAABB3D(const physics::AABB3D& aabb3D);

		void SetHalfExtent(const math::Vector3& halfExtent);
		math::Vector3 GetHalfExtent() const;

		// Inherited via Collider3DComponent
		virtual dooms::physics::AABB3D ExtractLocalAABB3D() final;

		virtual physics::Collider* GetWorldCollider() final;
	};
}
