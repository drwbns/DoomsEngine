#include "LightManager.h"

#include <Rendering/Buffer/UniformBufferObject/UniformBlockOffsetInfo.h>
#include <Rendering/Buffer/UniformBufferObject/UniformBufferObjectManager.h>
#include <Rendering/Light/DirectionalLight.h>
#include <Rendering/Light/PointLight.h>

void dooms::graphics::LightManager::UpdateUniformBufferObject(const bool force)
{
	UniformBufferObject* const lightUBO = dooms::graphics::UniformBufferObjectManager::GetSingleton()->GetUniformBufferObject(LIGHT_DATA_CONSTANT_BUFFER_NAME);

	D_ASSERT(IsValid(lightUBO));

	if (IsValid(lightUBO))
	{
		if (force || (bmIsAmbientLightIntensityDirty == true))
		{
			D_ASSERT(dooms::graphics::UniformBufferObjectManager::GetSingleton()->GetUniformBufferObject(LIGHT_DATA_CONSTANT_BUFFER_NAME)->GetUniformVariableOffset("ambientLightIntensity") == graphics::eUniformBlock_Global::ambientLightIntensity);
			lightUBO->UpdateLocalBuffer((void*)&mAmbientLightIntensity, graphics::eUniformBlock_Global::ambientLightIntensity, sizeof(FLOAT32));
			bmIsAmbientLightIntensityDirty = false;
		}

		{
			const UINT64 dirLightCount = dooms::DirectionalLight::GetStaticElementCount();
			lightUBO->UpdateLocalBuffer((void*)&dirLightCount, graphics::eUniformBlock_Global::dirLightCount, sizeof(dirLightCount));
		}

		{
			const UINT64 pointLightCount = dooms::PointLight::GetStaticElementCount();
			lightUBO->UpdateLocalBuffer((void*)&pointLightCount, graphics::eUniformBlock_Global::pointLightCount, sizeof(pointLightCount));
		}
	}
}

FLOAT32 dooms::graphics::LightManager::GetAmbientLightIntensity() const
{
	return mAmbientLightIntensity;
}

void dooms::graphics::LightManager::SetAmbientLightIntensity(const FLOAT32 intensity)
{
	mAmbientLightIntensity = intensity;
	bmIsAmbientLightIntensityDirty = true;
}
