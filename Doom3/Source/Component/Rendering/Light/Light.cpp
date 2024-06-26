#include "Light.h"

#include <Rendering/Buffer/UniformBufferObject/UniformBufferObjectManager.h>

void dooms::Light::InitComponent()
{
	AddLocalDirtyToTransformDirtyReceiver(bmIsLightUboDirty);
}

void dooms::Light::UpdateComponent()
{

}

void dooms::Light::OnEndOfFrame_Component()
{

}

void dooms::Light::OnDestroy()
{
	Base::OnDestroy();

	RemoveFromUniformBufferObjectManager();
	bmIsLightUboDirty.SetDirty();
	UpdateUniformBufferObject(true);
	
}

void dooms::Light::OnActivated()
{
	Component::OnActivated();
}

void dooms::Light::OnDeActivated()
{
	Component::OnDeActivated();
	UpdateUniformBufferObject(true);
}

dooms::Light::~Light()
{
}

void dooms::Light::SetIntensity(FLOAT32 intensity)
{
	mIntensity = intensity;
}

void dooms::Light::SetIndirectMultiplier(FLOAT32 indirectMultiplier)
{
	mIndirectMultiplier = indirectMultiplier;
}

void dooms::Light::SetColor(const math::Vector4& color)
{
	mColor = color;
}

void dooms::Light::UpdateLight()
{
	bmIsLightUboDirty = true;
}

FLOAT32 dooms::Light::GetIntensity()
{
	return mIntensity;
}

FLOAT32 dooms::Light::GetIndirectMultiplier()
{
	return mIndirectMultiplier;
}

math::Vector4 dooms::Light::GetColor()
{
	return mColor;
}

math::Vector4 dooms::Light::GetRadiance()
{
	return mColor * mIntensity;
}

const math::Vector4& dooms::Light::GetColor() const
{
	return mColor;
}
