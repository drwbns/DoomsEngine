#pragma once

// Utility Header File ( Don't Edit this )
// SourceFilePath : __Source_Core_Misc_FrameDirtyChecker_FrameDirtyChecker_reflection_h


#ifdef __Source_Core_Misc_FrameDirtyChecker_FrameDirtyChecker_reflection_h

#error "__Source_Core_Misc_FrameDirtyChecker_FrameDirtyChecker_reflection_h already included, missing '#pragma once' in __Source_Core_Misc_FrameDirtyChecker_FrameDirtyChecker_reflection_h"

#endif

#define __Source_Core_Misc_FrameDirtyChecker_FrameDirtyChecker_reflection_h


#include <type_traits>
#include <cassert>


//-------------------------------------------


#ifdef GENERATE_BODY_FULLNAME_dooms__FrameDirtyChecker
#error "GENERATE_BODY_FULLNAME_dooms__FrameDirtyChecker already included...."
#endif


#undef CURRENT_TYPE_ALIAS_dooms__FrameDirtyChecker
#define CURRENT_TYPE_ALIAS_dooms__FrameDirtyChecker \
public : \
typedef dooms::FrameDirtyChecker Current;


#undef TYPE_FULLNAME_HASH_VALUE_NAME_STRING_dooms__FrameDirtyChecker
#define TYPE_FULLNAME_HASH_VALUE_NAME_STRING_dooms__FrameDirtyChecker \
public : \
inline static const unsigned long int TYPE_FULL_NAME_HASH_VALUE = 1408350361; \
inline static const char* const TYPE_FULL_NAME = "dooms::FrameDirtyChecker"; \
inline static const char* const TYPE_SHORT_NAME = "FrameDirtyChecker"; \
virtual unsigned long int GetTypeHashVlue() const { return TYPE_FULL_NAME_HASH_VALUE; } \
virtual const char* GetTypeFullName() const { return TYPE_FULL_NAME; } \
virtual const char* GetTypeShortName() const { return TYPE_SHORT_NAME; }


#undef TYPE_CHECK_FUNCTION_FrameDirtyChecker
#define TYPE_CHECK_FUNCTION_FrameDirtyChecker \
private : \
attrNoReflect void __TYPE_CHECK() { static_assert(std::is_same_v<std::remove_reference<decltype(*this)>::type, Current> == true, "ERROR : WRONG TYPE. Please Check GENERATED_~ MACROS");} \


#undef GENERATE_BODY_FULLNAME_dooms__FrameDirtyChecker
#define GENERATE_BODY_FULLNAME_dooms__FrameDirtyChecker(...) \
CURRENT_TYPE_ALIAS_dooms__FrameDirtyChecker \
TYPE_FULLNAME_HASH_VALUE_NAME_STRING_dooms__FrameDirtyChecker \
TYPE_CHECK_FUNCTION_FrameDirtyChecker \
private:


//Type Short Name ( without namespace, only type name ) Version Macros.
#define GENERATE_BODY_FrameDirtyChecker(...) GENERATE_BODY_FULLNAME_dooms__FrameDirtyChecker(__VA_ARGS__)


#undef GENERATE_BODY
#define GENERATE_BODY(...) GENERATE_BODY_FULLNAME_dooms__FrameDirtyChecker(__VA_ARGS__)


//-------------------------------------------


