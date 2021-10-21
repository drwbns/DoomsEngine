#pragma once

#include <string>

#include <Macros/TypeDef.h>
#include <Macros/DllMarcos.h>

#include "DObjectMacros.h"
#include "DObject_Constant.h"

namespace doom
{
	class DObjectManager;
	class DObject;

	struct DOOM_API DObjectContructorParams
	{
		UINT32 DObjectFlag = 0;
		std::string mDObjectName;

		DObjectContructorParams() = default;
		DObjectContructorParams(const DObjectContructorParams&) = default;
		DObjectContructorParams(DObjectContructorParams&&) noexcept = default;
		DObjectContructorParams& operator=(const DObjectContructorParams&) = default;
		DObjectContructorParams& operator=(DObjectContructorParams&&) noexcept = default;
		~DObjectContructorParams() = default;
	};



	enum eDObjectFlag : UINT32
	{
		NewAllocated = 1 << 0
	};

	
	class DOOM_API DObject
	{
		DOBJECT_ABSTRACT_CLASS_BODY(DObject);
		
	public:
		FORCE_INLINE constexpr static SIZE_T BASE_CHAIN_COUNT_STATIC()
		{
			return 0;
		}
		FORCE_INLINE constexpr static const char* const * BASE_CHAIN_DATA_STATIC()
		{
			return nullptr;
		}
		virtual SIZE_T GetBaseChainCount() const { return BASE_CHAIN_COUNT_STATIC(); }
		virtual const char* const * GetBaseChainData() const { return BASE_CHAIN_DATA_STATIC(); }


		template <typename BASE_TYPE>
		FORCE_INLINE bool IsChildOf() const
		{
			static_assert(IS_DOBJECT_TYPE(BASE_TYPE));
			
			const bool isChild = ( GetBaseChainCount() >= BASE_TYPE::BASE_CHAIN_COUNT_STATIC() ) && ( GetBaseChainData()[GetBaseChainCount() - BASE_TYPE::BASE_CHAIN_COUNT_STATIC()] == BASE_TYPE::CLASS_TYPE_ID_STATIC() );

			return isChild;
		}

		friend class DObjectManager;

		struct DObjectProperties
		{
			UINT32 mDObjectFlag = 0;
			//Used For Debugging
			std::string mDObjectName;
			const DObject* mOwnerDObject = nullptr;

			DObjectProperties() = default;
			DObjectProperties(const DObjectProperties&) = default;
			DObjectProperties(DObjectProperties&&) noexcept = default;
			DObjectProperties& operator=(const DObjectProperties&) = default;
			DObjectProperties& operator=(DObjectProperties&&) noexcept = default;
			~DObjectProperties() = default;
		};

	private:

		UINT64 mDObjectID;
		DObjectProperties mDObjectProperties;

		void Construct_Internal();
		

	protected:

		DObject();
		DObject(const std::string& dObjectName);
		DObject(const DObject* const ownerDObject, const std::string& dObjectName);

		DObject(const DObjectContructorParams& params);
		DObject(const DObject& dObject);
		DObject(DObject&& dObject) noexcept;
		DObject& operator=(const DObject& dObject);
		DObject& operator=(DObject&& dObject) noexcept;
		virtual ~DObject();



	public:

		void InitProperties(const DObjectContructorParams& params);

		inline SIZE_T GetDObjectID() const
		{
			return mDObjectID;
		}

		inline UINT32 GetDObjectFlag() const
		{
			return mDObjectProperties.mDObjectFlag;
		}

		inline bool GetDObjectFlag(const eDObjectFlag flag) const
		{
			return (mDObjectProperties.mDObjectFlag & flag) != 0;
		}

		void ChangeDObjectName(const std::string& dObjectName);
		void SetOwnerDObject(const DObject* const ownerDObject);
	};
}

