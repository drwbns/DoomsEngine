#pragma once

// Utility Header File ( Don't Edit this )
// SourceFilePath : C:\Doom3FromScratch\Doom3\Source\Core\Graphics\DebugGraphics\DebugDrawer.cpp


#ifdef C__Doom3FromScratch_Doom3_Source_Core_Graphics_DebugGraphics_DebugDrawer_reflection_h

#error "C:\Doom3FromScratch\Doom3\Source\Core\Graphics\DebugGraphics\DebugDrawer.reflection.h already included, missing '#pragma once' in C:\Doom3FromScratch\Doom3\Source\Core\Graphics\DebugGraphics\DebugDrawer.reflection.h"

#endif

#define C__Doom3FromScratch_Doom3_Source_Core_Graphics_DebugGraphics_DebugDrawer_reflection_h


#include <type_traits>
#include <cassert>


//-------------------------------------------


#ifdef GENERATE_BODY_FULLNAME_dooms__graphics__DebugDrawer
#error "GENERATE_BODY_FULLNAME_dooms__graphics__DebugDrawer already included...."
#endif

#undef INHERITANCE_INFORMATION_dooms__graphics__DebugDrawer
#define INHERITANCE_INFORMATION_dooms__graphics__DebugDrawer \
public : \
inline static const unsigned long int BASE_CHAIN_LIST[] { 2625426556, 3969188510 }; \
inline static const unsigned long int BASE_CHAIN_LIST_LENGTH { 2 }; \
virtual const unsigned long int* GetBaseChainList() const { return BASE_CHAIN_LIST; } \
virtual unsigned long int GetBaseChainListLength() const { return BASE_CHAIN_LIST_LENGTH; } \
typedef dooms::DObject Base;


#undef CLONE_OBJECT_dooms__graphics__DebugDrawer
#define CLONE_OBJECT_dooms__graphics__DebugDrawer \
public : \
virtual dooms::DObject* CloneObject() const \
{ \
	dooms::DObject* clonedObject = nullptr; \
	/* std::vector<std::unique_ptr> can make false positive for std::is_copy_constructible<std::vector<std::unique_ptr>>::value. So Please explicitly delete copy constructor if you have this type variable */ \
	if constexpr( (std::is_copy_constructible<dooms::graphics::DebugDrawer>::value == true) && (std::is_base_of<dooms::DObject, dooms::graphics::DebugDrawer>::value == true) ) \
	{ \
		 clonedObject = dooms::CreateDObject<dooms::graphics::DebugDrawer>(*this); \
	} \
	assert(clonedObject != nullptr);	\
	return clonedObject;	\
}


#undef CURRENT_TYPE_ALIAS_dooms__graphics__DebugDrawer
#define CURRENT_TYPE_ALIAS_dooms__graphics__DebugDrawer \
public : \
typedef dooms::graphics::DebugDrawer Current;


#undef TYPE_FULLNAME_HASH_VALUE_NAME_STRING_dooms__graphics__DebugDrawer
#define TYPE_FULLNAME_HASH_VALUE_NAME_STRING_dooms__graphics__DebugDrawer \
public : \
inline static const unsigned long int TYPE_FULL_NAME_HASH_VALUE = 2625426556; \
inline static const char* const TYPE_FULL_NAME = "dooms::graphics::DebugDrawer"; \
inline static const char* const TYPE_SHORT_NAME = "DebugDrawer"; \
virtual unsigned long int GetTypeHashVlue() const { return TYPE_FULL_NAME_HASH_VALUE; } \
virtual const char* GetTypeFullName() const { return TYPE_FULL_NAME; } \
virtual const char* GetTypeShortName() const { return TYPE_SHORT_NAME; }


#undef TYPE_CHECK_FUNCTION_DebugDrawer
#define TYPE_CHECK_FUNCTION_DebugDrawer \
private : \
attrNoReflect void __TYPE_CHECK() { static_assert(std::is_same_v<std::remove_reference<decltype(*this)>::type, Current> == true, "ERROR : WRONG TYPE. Please Check GENERATED_~ MACROS");} \


#undef GENERATE_BODY_FULLNAME_dooms__graphics__DebugDrawer
#define GENERATE_BODY_FULLNAME_dooms__graphics__DebugDrawer(...) \
INHERITANCE_INFORMATION_dooms__graphics__DebugDrawer \
CLONE_OBJECT_dooms__graphics__DebugDrawer \
CURRENT_TYPE_ALIAS_dooms__graphics__DebugDrawer \
TYPE_FULLNAME_HASH_VALUE_NAME_STRING_dooms__graphics__DebugDrawer \
TYPE_CHECK_FUNCTION_DebugDrawer \
private:


//Type Short Name ( without namespace, only type name ) Version Macros.
#define GENERATE_BODY_DebugDrawer(...) GENERATE_BODY_FULLNAME_dooms__graphics__DebugDrawer(__VA_ARGS__)


#undef GENERATE_BODY
#define GENERATE_BODY(...) GENERATE_BODY_FULLNAME_dooms__graphics__DebugDrawer(__VA_ARGS__)


//-------------------------------------------

