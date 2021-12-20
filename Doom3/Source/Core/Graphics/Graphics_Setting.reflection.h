#pragma once

// Utility Header File ( Don't Edit this )
// SourceFilePath : __Source_Core_Graphics_Graphics_Setting_reflection_h


#ifdef __Source_Core_Graphics_Graphics_Setting_reflection_h

#error "__Source_Core_Graphics_Graphics_Setting_reflection_h already included, missing '#pragma once' in __Source_Core_Graphics_Graphics_Setting_reflection_h"

#endif

#define __Source_Core_Graphics_Graphics_Setting_reflection_h


#include <type_traits>
#include <cassert>


//-------------------------------------------


#ifdef GENERATE_BODY_FULLNAME_dooms__graphics__Graphics_Setting
#error "GENERATE_BODY_FULLNAME_dooms__graphics__Graphics_Setting already included...."
#endif


#undef CURRENT_TYPE_ALIAS_dooms__graphics__Graphics_Setting
#define CURRENT_TYPE_ALIAS_dooms__graphics__Graphics_Setting \
public : \
typedef dooms::graphics::Graphics_Setting Current;


#undef TYPE_FULLNAME_HASH_VALUE_NAME_STRING_dooms__graphics__Graphics_Setting
#define TYPE_FULLNAME_HASH_VALUE_NAME_STRING_dooms__graphics__Graphics_Setting \
public : \
inline static const unsigned long int TYPE_FULL_NAME_HASH_VALUE = 1775195333; \
inline static const char* const TYPE_FULL_NAME = "dooms::graphics::Graphics_Setting"; \
inline static const char* const TYPE_SHORT_NAME = "Graphics_Setting"; \
virtual unsigned long int GetTypeHashVlue() const { return TYPE_FULL_NAME_HASH_VALUE; } \
virtual const char* GetTypeFullName() const { return TYPE_FULL_NAME; } \
virtual const char* GetTypeShortName() const { return TYPE_SHORT_NAME; }


#undef TYPE_CHECK_FUNCTION_Graphics_Setting
#define TYPE_CHECK_FUNCTION_Graphics_Setting \
private : \
attrNoReflect void __TYPE_CHECK() { static_assert(std::is_same_v<std::remove_reference<decltype(*this)>::type, Current> == true, "ERROR : WRONG TYPE. Please Check GENERATED_~ MACROS");} \


#undef GENERATE_BODY_FULLNAME_dooms__graphics__Graphics_Setting
#define GENERATE_BODY_FULLNAME_dooms__graphics__Graphics_Setting(...) \
CURRENT_TYPE_ALIAS_dooms__graphics__Graphics_Setting \
TYPE_FULLNAME_HASH_VALUE_NAME_STRING_dooms__graphics__Graphics_Setting \
TYPE_CHECK_FUNCTION_Graphics_Setting \
private:


//Type Short Name ( without namespace, only type name ) Version Macros.
#define GENERATE_BODY_Graphics_Setting(...) GENERATE_BODY_FULLNAME_dooms__graphics__Graphics_Setting(__VA_ARGS__)


#undef GENERATE_BODY
#define GENERATE_BODY(...) GENERATE_BODY_FULLNAME_dooms__graphics__Graphics_Setting(__VA_ARGS__)


//-------------------------------------------


