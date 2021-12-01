#include "PIPManager.h"

#include "PicktureInPickture.h"

#include <vector_erase_move_lastelement/vector_swap_popback.h>

dooms::graphics::PIPManager::PIPManager()
	: mPicktureInPicktures()
{
}

void dooms::graphics::PIPManager::DrawPIPs()
{
	if (GetPIPCount() > 0)
	{
		D_START_PROFILING(DrawPIPs, dooms::profiler::eProfileLayers::Rendering);
		for (std::unique_ptr<PicktureInPickture>& pip : mPicktureInPicktures)
		{
			pip->DrawPictureInPicture();
		}
		D_END_PROFILING(DrawPIPs);
	}	
}

dooms::graphics::PicktureInPickture* dooms::graphics::PIPManager::AddNewPIP(const math::Vector2& leftBottomNDCPoint, const math::Vector2& rightTopNDCPoint, SingleTexture* const _drawedTexture)
{
	dooms::graphics::PicktureInPickture* pip = nullptr;

	if(_drawedTexture != nullptr)
	{
		std::unique_ptr<PicktureInPickture>& newPIP = mPicktureInPicktures.emplace_back(std::make_unique< dooms::graphics::PicktureInPickture>(leftBottomNDCPoint, rightTopNDCPoint, _drawedTexture));
		pip = newPIP.get();
	}

	D_ASSERT(pip != nullptr);
	return pip;
}

void dooms::graphics::PIPManager::RemovePIP(const PicktureInPickture* const removedPIP)
{
	swap_popback::vector_find_if_swap_popback(mPicktureInPicktures, [removedPIP](const std::unique_ptr<PicktureInPickture>& pip) -> bool
		{
			return pip.get() == removedPIP;
		});
}

size_t dooms::graphics::PIPManager::GetPIPCount() const
{
	return mPicktureInPicktures.size();
}
