#include "ColliderComponent.h"

#include <Entity.h>
#include <Rendering/Renderer/Renderer.h>

void doom::ColliderComponent::ResetAllCollisionState()
{
	this->mbIsColliderEnter = false;
	this->mbIsColliderExit = false;
	this->mbIsColliderStay = false;

	this->mbIsTriggerEnter = false;
	this->mbIsTriggerExit = false;
	this->mbIsTriggerStay = false;
}

void doom::ColliderComponent::InitComponent()
{
	this->AddLocalDirtyToTransformDirtyReceiver(this->bmIsWorldColliderDirty);

	this->AutoColliderSetting();
}

void doom::ColliderComponent::UpdateComponent()
{
	this->UpdatePhysics();
}

void doom::ColliderComponent::FixedUpdateComponent()
{
}

void doom::ColliderComponent::OnEndOfFrame_Component()
{

}

void doom::ColliderComponent::OnDestroy()
{
	this->ResetAllCollisionState();
}

void doom::ColliderComponent::OnActivated()
{
	this->ResetAllCollisionState();
}

void doom::ColliderComponent::OnDeActivated()
{
	this->ResetAllCollisionState();
}

bool doom::ColliderComponent::GetMeshAABB3D(physics::AABB3D& aabb3D)
{
	const Renderer* renderer = this->GetOwnerEntity()->GetComponent<Renderer>();
	if (renderer != nullptr)
	{
		aabb3D = renderer->GetLocalAABB3D();
		return true;
	}
	else
	{
		return false;
	}
}

void doom::ColliderComponent::OnPreUpdatePhysics()
{
	this->ResetAllCollisionState();

	if (this->bmIsLocalColliderDirty.GetIsDirty(true))
	{
		this->UpdateLocalCollider();
		this->bmIsWorldColliderDirty.SetDirty(true);
	}

	if (this->bmIsWorldColliderDirty.GetIsDirty(true))
	{
		this->UpdateWorldCollider();
	}
}

void doom::ColliderComponent::UpdatePhysics()
{
}

void doom::ColliderComponent::OnPostUpdatePhysics()
{
}
