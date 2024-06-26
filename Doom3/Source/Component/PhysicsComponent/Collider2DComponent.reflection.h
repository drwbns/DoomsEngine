#pragma once

// Utility Header File ( Don't Edit this )
// SourceFilePath : __Source_Component_PhysicsComponent_Collider2DComponent_reflection_h


#ifdef __Source_Component_PhysicsComponent_Collider2DComponent_reflection_h

#error "__Source_Component_PhysicsComponent_Collider2DComponent_reflection_h already included, missing '#pragma once' in __Source_Component_PhysicsComponent_Collider2DComponent_reflection_h"

#endif

#define __Source_Component_PhysicsComponent_Collider2DComponent_reflection_h


#include <type_traits>
#include <cassert>


//-------------------------------------------


#ifdef GENERATE_BODY_FULLNAME_dooms__Collider2DComponent
#error "GENERATE_BODY_FULLNAME_dooms__Collider2DComponent already included...."
#endif

#undef INHERITANCE_INFORMATION_dooms__Collider2DComponent
#define INHERITANCE_INFORMATION_dooms__Collider2DComponent \
public : \
inline static const unsigned long int BASE_CHAIN_LIST[] { 2836413646, 960295289, 3040581954, 3969188510 }; \
inline static const unsigned long int BASE_CHAIN_LIST_LENGTH { 4 }; \
virtual const unsigned long int* GetBaseChainList() const { return BASE_CHAIN_LIST; } \
virtual unsigned long int GetBaseChainListLength() const { return BASE_CHAIN_LIST_LENGTH; } \
typedef dooms::ColliderComponent Base;


#undef CLONE_OBJECT_dooms__Collider2DComponent
#define CLONE_OBJECT_dooms__Collider2DComponent \
public : \
virtual dooms::DObject* CloneObject() const \
{ \
	dooms::DObject* clonedObject = nullptr; \
	/* std::vector<std::unique_ptr> can make false positive for std::is_copy_constructible<std::vector<std::unique_ptr>>::value. So Please explicitly delete copy constructor if you have this type variable */ \
	if constexpr( (std::is_copy_constructible<dooms::Collider2DComponent>::value == true) && (std::is_base_of<dooms::DObject, dooms::Collider2DComponent>::value == true) ) \
	{ \
		 clonedObject = dooms::CreateDObject<dooms::Collider2DComponent>(*this); \
	} \
	assert(clonedObject != nullptr);	\
	return clonedObject;	\
}


#undef CURRENT_TYPE_ALIAS_dooms__Collider2DComponent
#define CURRENT_TYPE_ALIAS_dooms__Collider2DComponent \
public : \
typedef dooms::Collider2DComponent Current;


#undef TYPE_FULLNAME_HASH_VALUE_NAME_STRING_dooms__Collider2DComponent
#define TYPE_FULLNAME_HASH_VALUE_NAME_STRING_dooms__Collider2DComponent \
public : \
inline static const unsigned long int TYPE_FULL_NAME_HASH_VALUE = 2836413646; \
inline static const char* const TYPE_FULL_NAME = "dooms::Collider2DComponent"; \
inline static const char* const TYPE_SHORT_NAME = "Collider2DComponent"; \
virtual unsigned long int GetTypeHashVlue() const { return TYPE_FULL_NAME_HASH_VALUE; } \
virtual const char* GetTypeFullName() const { return TYPE_FULL_NAME; } \
virtual const char* GetTypeShortName() const { return TYPE_SHORT_NAME; }


#undef TYPE_CHECK_FUNCTION_Collider2DComponent
#define TYPE_CHECK_FUNCTION_Collider2DComponent \
private : \
attrNoReflect void __TYPE_CHECK() { static_assert(std::is_same_v<std::remove_reference<decltype(*this)>::type, Current> == true, "ERROR : WRONG TYPE. Please Check GENERATED_~ MACROS");} \


#undef GENERATE_BODY_FULLNAME_dooms__Collider2DComponent
#define GENERATE_BODY_FULLNAME_dooms__Collider2DComponent(...) \
INHERITANCE_INFORMATION_dooms__Collider2DComponent \
CLONE_OBJECT_dooms__Collider2DComponent \
CURRENT_TYPE_ALIAS_dooms__Collider2DComponent \
TYPE_FULLNAME_HASH_VALUE_NAME_STRING_dooms__Collider2DComponent \
TYPE_CHECK_FUNCTION_Collider2DComponent \
private:


//Type Short Name ( without namespace, only type name ) Version Macros.
#define GENERATE_BODY_Collider2DComponent(...) GENERATE_BODY_FULLNAME_dooms__Collider2DComponent(__VA_ARGS__)


#undef GENERATE_BODY
#define GENERATE_BODY(...) GENERATE_BODY_FULLNAME_dooms__Collider2DComponent(__VA_ARGS__)


//-------------------------------------------


