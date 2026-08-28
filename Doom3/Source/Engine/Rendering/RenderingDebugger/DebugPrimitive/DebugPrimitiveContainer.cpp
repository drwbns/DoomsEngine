#include "DebugPrimitiveContainer.h"

#include <cstring>
#include <unordered_map>

namespace
{
	// Colours are matched at 8 bits per channel. Anything closer than that is
	// indistinguishable on screen, and quantising is what lets a colour ramp
	// collapse into a handful of batches instead of one per primitive.
	UINT32 PackColorKey(const math::Vector4& color)
	{
		const auto channel = [](const FLOAT32 value) -> UINT32
		{
			const FLOAT32 clamped = (value < 0.0f) ? 0.0f : ((value > 1.0f) ? 1.0f : value);
			return static_cast<UINT32>(clamped * 255.0f + 0.5f);
		};

		return (channel(color.x) << 24) | (channel(color.y) << 16) | (channel(color.z) << 8) | channel(color.w);
	}
}

std::vector<float>& dooms::graphics::DebugPrimitiveContainer::GetColoredVertexVector(const eColor color)
{
	D_ASSERT(static_cast<UINT32>(color) < ENUM_COLOR_COUNT);

	return mColoredVertexData[static_cast<UINT32>(color)];
}

size_t dooms::graphics::DebugPrimitiveContainer::GetComponentSize() const
{
	return sizeof(FLOAT32);
}

const float* dooms::graphics::DebugPrimitiveContainer::GetColoredVertexData(const eColor color) const
{
	D_ASSERT(static_cast<UINT32>(color) < ENUM_COLOR_COUNT);

	return mColoredVertexData[static_cast<UINT32>(color)].data();
}

/*
size_t dooms::graphics::DebugPrimitiveContainer::GetColoredVertexCount(const eColor color) const
{
	D_ASSERT(static_cast<UINT32>(color) < ENUM_COLOR_COUNT);

	return mColoredVertexData[static_cast<UINT32>(color)].size() / GetVertexCountPerPrimitive();
}
*/

size_t dooms::graphics::DebugPrimitiveContainer::GetColoredPrimitiveCount(const eColor color) const
{
	D_ASSERT(static_cast<UINT32>(color) < ENUM_COLOR_COUNT);

	return mColoredVertexData[static_cast<UINT32>(color)].size() / GetComponentCountPerPrimitive();
}

const float* dooms::graphics::DebugPrimitiveContainer::GetSpecialColoredVertexData() const
{
	return mSpecialColoredVertexData.data();
}

size_t dooms::graphics::DebugPrimitiveContainer::GetSpecialColoredVertexDataCount() const
{
	return mSpecialColoredVertexData.size() / GetComponentCountPerPrimitive();
}

const math::Vector4* dooms::graphics::DebugPrimitiveContainer::GetSpecialColorData() const
{
	return mSpecialColorData.data();
}

size_t dooms::graphics::DebugPrimitiveContainer::GetSpecialColorDataCount() const
{
	return mSpecialColorData.size();
}

size_t dooms::graphics::DebugPrimitiveContainer::GetSpecialColoredPrimitiveCount() const
{
	return mSpecialColoredVertexData.size() / GetComponentCountPerPrimitive();
}

void dooms::graphics::DebugPrimitiveContainer::BuildSpecialColorBatches()
{
	if (bmIsSpecialColorBatchesBuilt == true)
	{
		return;
	}

	bmIsSpecialColorBatchesBuilt = true;

	mSpecialColorBatches.clear();
	mBatchedSpecialColoredVertexData.clear();

	if (mSpecialColorData.empty() == true)
	{
		return;
	}

	const size_t componentCountPerPrimitive = GetComponentCountPerPrimitive();

	// Driven by the vertex data rather than the colour list, because the vertex
	// data is what BufferVertexDataToGPU sizes its upload from. The two must
	// agree, or the batches would describe a different span than was uploaded.
	const size_t primitiveCount = GetSpecialColoredPrimitiveCount();
	D_ASSERT(primitiveCount == mSpecialColorData.size());

	// Batches are created in first-seen order so that what ends up on screen
	// stays stable from frame to frame rather than shuffling with the hashing.
	std::unordered_map<UINT32, size_t> batchIndexFromColorKey;
	std::vector<std::vector<UINT32>> primitiveIndicesPerBatch;

	for (size_t primitiveIndex = 0; primitiveIndex < primitiveCount; primitiveIndex++)
	{
		const UINT32 colorKey = PackColorKey(mSpecialColorData[primitiveIndex]);
		const auto foundBatch = batchIndexFromColorKey.find(colorKey);

		if (foundBatch == batchIndexFromColorKey.cend())
		{
			batchIndexFromColorKey.emplace(colorKey, mSpecialColorBatches.size());
			mSpecialColorBatches.push_back(SpecialColorBatch{ mSpecialColorData[primitiveIndex], 0 });
			primitiveIndicesPerBatch.emplace_back();
			primitiveIndicesPerBatch.back().push_back(static_cast<UINT32>(primitiveIndex));
		}
		else
		{
			primitiveIndicesPerBatch[foundBatch->second].push_back(static_cast<UINT32>(primitiveIndex));
		}
	}

	mBatchedSpecialColoredVertexData.resize(mSpecialColoredVertexData.size());

	size_t writtenComponentCount = 0;
	for (size_t batchIndex = 0; batchIndex < mSpecialColorBatches.size(); batchIndex++)
	{
		const std::vector<UINT32>& primitiveIndices = primitiveIndicesPerBatch[batchIndex];

		for (const UINT32 primitiveIndex : primitiveIndices)
		{
			std::memcpy
			(
				mBatchedSpecialColoredVertexData.data() + writtenComponentCount,
				mSpecialColoredVertexData.data() + static_cast<size_t>(primitiveIndex) * componentCountPerPrimitive,
				componentCountPerPrimitive * sizeof(FLOAT32)
			);

			writtenComponentCount += componentCountPerPrimitive;
		}

		mSpecialColorBatches[batchIndex].mPrimitiveCount = static_cast<UINT32>(primitiveIndices.size());
	}
}

const float* dooms::graphics::DebugPrimitiveContainer::GetBatchedSpecialColoredVertexData() const
{
	return mBatchedSpecialColoredVertexData.data();
}

const std::vector<dooms::graphics::DebugPrimitiveContainer::SpecialColorBatch>& dooms::graphics::DebugPrimitiveContainer::GetSpecialColorBatches() const
{
	return mSpecialColorBatches;
}

void dooms::graphics::DebugPrimitiveContainer::ReserveVector(const size_t primitiveCount)
{
	for(auto& coloredVertexVector : mColoredVertexData)
	{
		coloredVertexVector.reserve(primitiveCount * GetComponentCountPerPrimitive());
	}

	mSpecialColoredVertexData.reserve(primitiveCount * GetComponentCountPerPrimitive());
	mSpecialColorData.reserve(primitiveCount * GetComponentCountPerPrimitive());
}

void dooms::graphics::DebugPrimitiveContainer::ClearDatas()
{
	for (auto& coloredVertexVector : mColoredVertexData)
	{
		coloredVertexVector.clear();
	}

	mSpecialColoredVertexData.clear();
	mSpecialColorData.clear();

	mBatchedSpecialColoredVertexData.clear();
	mSpecialColorBatches.clear();
	bmIsSpecialColorBatchesBuilt = false;
}

bool dooms::graphics::DebugPrimitiveContainer::IsColoredVertexDataEmpty(const eColor color) const
{
	D_ASSERT(static_cast<UINT32>(color) < ENUM_COLOR_COUNT);
	return mColoredVertexData[static_cast<UINT32>(color)].empty();
}

bool dooms::graphics::DebugPrimitiveContainer::IsSpecialColoredVertexDataEmpty() const
{
	return mSpecialColoredVertexData.empty();
}
