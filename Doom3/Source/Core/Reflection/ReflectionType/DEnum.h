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

		/// <summary>
		/// if value doesn't exist, return nullptr
		/// </summary>
		/// <param name="value"></param>
		/// <param name="primitiveNameType"></param>
		/// <returns></returns>
		FORCE_INLINE const char* GetValueName(const INT32 value, const dooms::ePrimitiveNameType primitiveNameType = dooms::ePrimitiveNameType::Short) const
		{
			const char* valueName = nullptr;

			if(primitiveNameType == dooms::ePrimitiveNameType::Full)
			{
				valueName = clEnum->GetValueName(value);
			}
			else if (primitiveNameType == dooms::ePrimitiveNameType::Short)
			{
				valueName = dPrimitiveHelper::GetShortNamePointer(clEnum->GetValueName(value));
			}

			return valueName;
		}

		// if value is found, return true
		const bool GetValue(const char* const valueName, INT32& result) const;
	};

	template <typename T>
	extern DEnum CreateDEnum()
	{
		return DEnum(clcpp::GetTypeNameHash<T>());
	}
}