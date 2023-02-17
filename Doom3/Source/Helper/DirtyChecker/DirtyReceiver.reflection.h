#pragma once

// Utility Header File ( Don't Edit this )
// SourceFilePath : __Source_Helper_DirtyChecker_DirtyReceiver_reflection_h


#ifdef __Source_Helper_DirtyChecker_DirtyReceiver_reflection_h

#error "__Source_Helper_DirtyChecker_DirtyReceiver_reflection_h already included, missing '#pragma once' in __Source_Helper_DirtyChecker_DirtyReceiver_reflection_h"

#endif

#define __Source_Helper_DirtyChecker_DirtyReceiver_reflection_h


#include <type_traits>
#include <cassert>


//-------------------------------------------


#ifdef GENERATE_BODY_FULLNAME_DirtyReceiver
#error "GENERATE_BODY_FULLNAME_DirtyReceiver already included...."
#endif


#undef CURRENT_TYPE_ALIAS_DirtyReceiver
#define CURRENT_TYPE_ALIAS_DirtyReceiver \
public : \
typedef DirtyReceiver Current;


#undef TYPE_FULLNAME_HASH_VALUE_NAME_STRING_DirtyReceiver
#define TYPE_FULLNAME_HASH_VALUE_NAME_STRING_DirtyReceiver \
public : \
inline static const unsigned long int TYPE_FULL_NAME_HASH_VALUE = 1119909272; \
inline static const char* const TYPE_FULL_NAME = "DirtyReceiver"; \
inline static const char* const TYPE_SHORT_NAME = "DirtyReceiver"; \
virtual unsigned long int GetTypeHashVlue() const { return TYPE_FULL_NAME_HASH_VALUE; } \
virtual const char* GetTypeFullName() const { return TYPE_FULL_NAME; } \
virtual const char* GetTypeShortName() const { return TYPE_SHORT_NAME; }


#undef TYPE_CHECK_FUNCTION_DirtyReceiver
#define TYPE_CHECK_FUNCTION_DirtyReceiver \
private : \
attrNoReflect void __TYPE_CHECK() { static_assert(std::is_same_v<std::remove_reference<decltype(*this)>::type, Current> == true, "ERROR : WRONG TYPE. Please Check GENERATED_~ MACROS");} \


#undef GENERATE_BODY_FULLNAME_DirtyReceiver
#define GENERATE_BODY_FULLNAME_DirtyReceiver(...) \
CURRENT_TYPE_ALIAS_DirtyReceiver \
TYPE_FULLNAME_HASH_VALUE_NAME_STRING_DirtyReceiver \
TYPE_CHECK_FUNCTION_DirtyReceiver \
private:


//Type Short Name ( without namespace, only type name ) Version Macros.
#define GENERATE_BODY_DirtyReceiver(...) GENERATE_BODY_FULLNAME_DirtyReceiver(__VA_ARGS__)


#undef GENERATE_BODY
#define GENERATE_BODY(...) GENERATE_BODY_FULLNAME_DirtyReceiver(__VA_ARGS__)


//-------------------------------------------

