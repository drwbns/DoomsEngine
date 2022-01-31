#include "UniformBufferObjectManager.h"

#include "UniformBufferObjectUpdater.h"
#include "../../../Helper/vector_erase_move_lastelement/vector_swap_popback.h"

void dooms::graphics::UniformBufferObjectManager::UpdateUniformBufferObjects()
{
	for (UniformBufferObjectUpdater* updater : mUniformBufferObjectTempBufferUpdaters)
	{
		if (updater->GetIsUpdateWhenManagerUpdate() == true)
		{
			updater->UpdateUniformBufferObject();
		}
	}
}

void dooms::graphics::UniformBufferObjectManager::PushUniformBufferObjectTempBufferUpdater(UniformBufferObjectUpdater* update_ptr)
{
	mUniformBufferObjectTempBufferUpdaters.push_back(update_ptr);
}

void dooms::graphics::UniformBufferObjectManager::EraseUniformBufferObjectTempBufferUpdater(UniformBufferObjectUpdater* const update_ptr)
{
	auto iter_end = mUniformBufferObjectTempBufferUpdaters.end();
	auto this_iter = std::find_if(mUniformBufferObjectTempBufferUpdaters.begin(),
		iter_end,
		[update_ptr](const UniformBufferObjectUpdater* stored_update_ptr) {return stored_update_ptr == update_ptr; });

	if(this_iter != iter_end)
	{
		swap_popback::vector_swap_popback(mUniformBufferObjectTempBufferUpdaters, this_iter);
	}
}

void dooms::graphics::UniformBufferObjectManager::UpdateUniformObjects()
{
	UpdateUniformBufferObjects();
	BufferDateOfUniformBufferObjects();
}

dooms::graphics::UniformBufferObject* dooms::graphics::UniformBufferObjectManager::GetUniformBufferObject(const char* const uniformBufferName)
{
	dooms::graphics::UniformBufferObject* uniformBufferObject = nullptr;

	auto uniformBufferObjectNode = mUniformBufferObjects.find(uniformBufferName);
	D_ASSERT(uniformBufferObjectNode != mUniformBufferObjects.end());
	if (uniformBufferObjectNode != mUniformBufferObjects.end())
	{
		uniformBufferObject = uniformBufferObjectNode->second;
	}

	D_ASSERT(IsValid(uniformBufferObject));

	return uniformBufferObject;
}


void dooms::graphics::UniformBufferObjectManager::BufferDateOfUniformBufferObjects()
{
	for (auto& uniformBufferObjectNode : mUniformBufferObjects)
	{
		dooms::graphics::UniformBufferObject* uniformBufferObject = uniformBufferObjectNode.second;
		D_ASSERT(IsValid(uniformBufferObject));

		if(IsValid(uniformBufferObject))
		{
			if (uniformBufferObject->IsBufferGenerated())
			{
				uniformBufferObject->UpdateLocalBufferToGPU();
			}
		}
		
	}
}


dooms::graphics::UniformBufferObject* dooms::graphics::UniformBufferObjectManager::GetOrGenerateUniformBufferObjectIfNotExist
(
	const std::string& uniformBufferName,
	const UINT64 uniformBufferSize,
	const UINT32 bindingPoint,
	const void* const initialData,
	const std::vector<asset::shaderReflectionDataParser::UniformBufferMember>* const uboMembers
)
{
	dooms::graphics::UniformBufferObject* ubo = nullptr;

	//D_ASSERT(mUniformBufferObjects.find(uniformBufferName) == mUniformBufferObjects.end());
	auto node = mUniformBufferObjects.find(uniformBufferName);
	if(node == mUniformBufferObjects.end())
	{
		ubo = dooms::CreateDObject<dooms::graphics::UniformBufferObject>(uniformBufferName, uniformBufferSize, bindingPoint, initialData, uboMembers);

		auto node = mUniformBufferObjects.emplace(uniformBufferName, ubo);
	}
	else
	{
		ubo = node->second;
	}

	D_ASSERT(IsValid(ubo));
	
	return ubo;
}


dooms::graphics::UniformBufferObjectManager::UniformBufferObjectManager()
{

}

