#pragma once

#include <vector>
#include <mutex>
#include <cassert>
#include <unordered_set>

#include <Macros/TypeDef.h>
#include <Macros/DllMarcos.h>
#include <CompilerMacros.h>

#include "DObject_Constant.h"

#define DEFUALT_DOBJECT_LIST_RESERVATION_SIZE 35000

#include "DObjectManager.reflection.h"
namespace dooms
{
	class DObject;

	namespace gc
	{
		class GarbageCollectorManager;
	}

	struct DObjectsContainer
	{
		friend class DObject;

		std::unordered_set<DObject*> mDObjectList;
		
		std::vector<UINT32> mDObjectFlagList;
		std::vector<UINT32> mEmptyIndexInFlagList;

		DObjectsContainer();
		DObjectsContainer(const DObjectsContainer&) = delete;
		DObjectsContainer(DObjectsContainer&&) noexcept = delete;
		DObjectsContainer& operator=(const DObjectsContainer&) = delete;
		DObjectsContainer& operator=(DObjectsContainer&&) noexcept = delete;

		bool IsEmpty() const;

		FORCE_INLINE UINT32 GetDObjectFlag(const size_t index) const
		{
			assert(index < mDObjectFlagList.size());
			return mDObjectFlagList[index];
		}

		FORCE_INLINE void SetDObjectFlag(const size_t index, const UINT32 flag)
		{
			assert(index < mDObjectFlagList.size());
			mDObjectFlagList[index] |= flag;
		}

		FORCE_INLINE void ClearDObjectFlag(const size_t index, const UINT32 flag)
		{
			assert(index < mDObjectFlagList.size());
			mDObjectFlagList[index] &= (~flag);
		}

		FORCE_INLINE void ResetDObjectFlag(const size_t index, const UINT32 flag)
		{
			assert(index < mDObjectFlagList.size());
			mDObjectFlagList[index] = flag;
		}
	};

	class DOOM_API /*D_CLASS*/ DObjectManager
	{
		//GENERATE_BODY()

		friend class DObject;
		friend class gc::GarbageCollectorManager;

	private:

		inline static std::atomic<UINT64> mDObjectCounter = 0;

		/// <summary>
		/// this is need to be optimized
		///	because searching is too slow.
		/// </summary>
		inline static DObjectsContainer mDObjectsContainer{};

		static UINT64 GenerateNewDObejctID();

		static void InsertDObjectID(DObject* const dObject, const UINT64 dObjectID);

		static bool AddNewDObject(DObject* const dObject);
		static bool ReplaceDObjectFromDObjectList(DObject&& originalDObject, DObject* const newDObject);
		static bool RemoveDObject(DObject* const dObject);

	public:

		inline static std::recursive_mutex DObjectListMutex{};

		static void DestroyAllDObjects(const bool force);
		static void ClearConatiner();

		/// <summary>
		/// Check if DObject address is valid
		///	this function check if passed pointer is null and if passed address is alive dObject address
		/// </summary>
		/// <param name="dObject"></param>
		/// <returns></returns>
		static bool IsDObjectStrongValid(const DObject* const dObject, const bool lock = true);
		static bool IsDObjectExist(const DObject* const dObject, const bool lock = true);

		static bool IsEmpty();

		static size_t GetDObjectCount();

	};
}
