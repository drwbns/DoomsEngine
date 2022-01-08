#include "Buffer.h"
#include "../GraphicsAPI/GraphicsAPI.h"

dooms::graphics::Buffer::Buffer() : mBufferID{ INVALID_BUFFER_ID }
{
}



void dooms::graphics::Buffer::GenBuffer()
{
	if (mBufferID == INVALID_BUFFER_ID)
	{
		mBufferID = GraphicsAPI::GenerateBuffer(1)[0];
	}

}

void dooms::graphics::Buffer::GenBufferIfNotGened()
{
	if (IsBufferGenerated() == true)
	{
		return;
	}
		
	GenBuffer();
}

/*
int64_t dooms::graphics::Buffer::GetBufferParameteri64v(eBufferBindingTarget bindingTarget, eBufferParameter bufferParameter)
{
	int64_t value;
	glGetBufferParameteri64v(static_cast<UINT32>(bindingTarget), static_cast<UINT32>(bufferParameter), &value);
	return value;
}

int64_t dooms::graphics::Buffer::GetNamedBufferParameteri64v(eBufferParameter bufferParameter)
{
	int64_t value;
	glGetNamedBufferParameteri64v(mBufferID, static_cast<UINT32>(bufferParameter), &value);
	return value;
}
*/

dooms::graphics::Buffer::~Buffer()
{
	DeleteBuffers();
}

void dooms::graphics::Buffer::DeleteBuffers()
{
	if (mBufferID.GetBufferID() != INVALID_BUFFER_ID)
	{
		GraphicsAPI::DestroyBuffer(mBufferID);
		mBufferID = 0;
	}
}

bool dooms::graphics::Buffer::IsBufferGenerated() const
{
	return mBufferID.GetBufferID() != INVALID_BUFFER_ID;
}

