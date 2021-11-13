#pragma once
#include "Collider3DComponent.h"
#include <Collider/CapsuleCollider.h>

#include "CapsuleCollider3D.reflection.h"
namespace dooms
{
	class DOOM_API D_CLASS CapsuleCollider3D : public Collider3DComponent
	{
		GENERATE_BODY()
		

		friend class physics::Physics_Server;
	private:

		physics::CapsuleCollider mLocalCapsuleCollider;
		physics::CapsuleCollider mWorldCapsuleCollider;

		FLOAT32 mHeight;
		FLOAT32 mRadius;

		virtual void UpdateLocalCollider() final;
		virtual void UpdateWorldCollider() final;

		void AutoColliderSettingFromAABB3D(const physics::AABB3D& aabb3dFromMesh) final;
		


	public:
	
		void SetHeight(FLOAT32 height);
		FLOAT32 GetHeight();
		void SetRadius(FLOAT32 radius);
		FLOAT32 GetRadius();
		
		// Inherited via Collider3DComponent
		virtual dooms::physics::AABB3D ExtractLocalAABB3D() override;

		virtual physics::Collider* GetWorldCollider() final;
	};
}
