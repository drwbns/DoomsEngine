#pragma once

#include <Macros/DllMarcos.h>
#include "../Reflection.h"

#include "DType.h"

D_NAMESPACE(dooms)
namespace dooms
{
	
	class DObject;
	class DOOM_API D_STRUCT DEnum : public DType /*: public dooms::DObject*/ // Dont Do this
	{
	protected:

		const clcpp::Enum* clEnum;

	public:

		DEnum(const UINT32 nameHash);

		FORCE_INLINE const char* GetValueName(const INT32 value)
		{
			return clEnum->GetValueName(value);
		}

		// if value is found, return true
		const bool GetValue(const char* const valueName, INT32& result);
	};


}