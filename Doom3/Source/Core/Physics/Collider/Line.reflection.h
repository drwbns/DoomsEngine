#pragma once

// Utility Header File ( Don't Edit this )
// SourceFilePath : C:\Doom3FromScratch\Doom3\Source\Core\Physics\Collider\Line.cpp


#ifdef C__Doom3FromScratch_Doom3_Source_Core_Physics_Collider_Line_reflection_h

#error "C:\Doom3FromScratch\Doom3\Source\Core\Physics\Collider\Line.reflection.h already included, missing '#pragma once' in C:\Doom3FromScratch\Doom3\Source\Core\Physics\Collider\Line.reflection.h"

#endif

#define C__Doom3FromScratch_Doom3_Source_Core_Physics_Collider_Line_reflection_h


#include <type_traits>
#include <cassert>


//-------------------------------------------


#ifdef GENERATE_BODY_FULLNAME_dooms__physics__Line
#error "GENERATE_BODY_FULLNAME_dooms__physics__Line already included...."
#endif


#undef CURRENT_TYPE_ALIAS_dooms__physics__Line
#define CURRENT_TYPE_ALIAS_dooms__physics__Line \
public : \
typedef dooms::physics::Line Current;


#undef TYPE_FULLNAME_HASH_VALUE_NAME_STRING_dooms__physics__Line
#define TYPE_FULLNAME_HASH_VALUE_NAME_STRING_dooms__physics__Line \
public : \
inline static const unsigned long int TYPE_FULL_NAME_HASH_VALUE = 1016117661; \
inline static const char* const TYPE_FULL_NAME = "dooms::physics::Line"; \
inline static const char* const TYPE_SHORT_NAME = "Line"; \
virtual unsigned long int GetTypeHashVlue() const { return TYPE_FULL_NAME_HASH_VALUE; } \
virtual const char* GetTypeFullName() const { return TYPE_FULL_NAME; } \
virtual const char* GetTypeShortName() const { return TYPE_SHORT_NAME; }


#undef TYPE_CHECK_FUNCTION_Line
#define TYPE_CHECK_FUNCTION_Line \
private : \
attrNoReflect void __TYPE_CHECK() { static_assert(std::is_same_v<std::remove_reference<decltype(*this)>::type, Current> == true, "ERROR : WRONG TYPE. Please Check GENERATED_~ MACROS");} \


#undef GENERATE_BODY_FULLNAME_dooms__physics__Line
#define GENERATE_BODY_FULLNAME_dooms__physics__Line(...) \
CURRENT_TYPE_ALIAS_dooms__physics__Line \
TYPE_FULLNAME_HASH_VALUE_NAME_STRING_dooms__physics__Line \
TYPE_CHECK_FUNCTION_Line \
private:


//Type Short Name ( without namespace, only type name ) Version Macros.
#define GENERATE_BODY_Line(...) GENERATE_BODY_FULLNAME_dooms__physics__Line(__VA_ARGS__)


#undef GENERATE_BODY
#define GENERATE_BODY(...) GENERATE_BODY_FULLNAME_dooms__physics__Line(__VA_ARGS__)


//-------------------------------------------

