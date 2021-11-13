#pragma once

// Utility Header File ( Don't Edit this )
// SourceFilePath : C:\Doom3FromScratch\Doom3\Source\Component\Core\Component.cpp


#ifdef C__Doom3FromScratch_Doom3_Source_Component_Core_Component_reflection_h

#error "C:\Doom3FromScratch\Doom3\Source\Component\Core\Component.reflection.h already included, missing '#pragma once' in C:\Doom3FromScratch\Doom3\Source\Component\Core\Component.reflection.h"

#endif

#define C__Doom3FromScratch_Doom3_Source_Component_Core_Component_reflection_h


#include <type_traits>


//-------------------------------------------


#ifdef GENERATE_BODY_FULLNAME_dooms__Component
#error "GENERATE_BODY_FULLNAME_dooms__Component already included...."
#endif

#undef INHERITANCE_INFORMATION_dooms__Component
#define INHERITANCE_INFORMATION_dooms__Component \
public: inline static const unsigned long int BASE_CHAIN_LIST[] { 3040581954, 3969188510 }; \
inline static const unsigned long int BASE_CHAIN_LIST_LENGTH { 2 }; \
virtual const unsigned long int* GetBastChainList() const { return BASE_CHAIN_LIST; } \
virtual unsigned long int GetBastChainListLength() const { return BASE_CHAIN_LIST_LENGTH; } \
public: typedef dooms::DObject Base;


#undef CURRENT_TYPE_ALIAS_dooms__Component
#define CURRENT_TYPE_ALIAS_dooms__Component \
public: typedef dooms::Component Current;


#undef TYPE_FULLNAME_HASH_VALUE_NAME_STRING_dooms__Component
#define TYPE_FULLNAME_HASH_VALUE_NAME_STRING_dooms__Component \
public: \
inline static const unsigned long int TYPE_FULL_NAME_HASH_VALUE = 3040581954; \
inline static const char* const TYPE_FULL_NAME = "dooms::Component"; \
inline static const char* const TYPE_SHORT_NAME = "Component"; 


#undef TYPE_CHECK_FUNCTION_Component
#define TYPE_CHECK_FUNCTION_Component \
private: \
attrNoReflect void __TYPE_CHECK() { static_assert(std::is_same_v<std::decay<decltype(*this)>::type, Current> == true, "ERROR : WRONG TYPE. Please Check GENERATED_~ MACROS");} \


#undef GENERATE_BODY_FULLNAME_dooms__Component
#define GENERATE_BODY_FULLNAME_dooms__Component(...) \
INHERITANCE_INFORMATION_dooms__Component \
CURRENT_TYPE_ALIAS_dooms__Component \
TYPE_FULLNAME_HASH_VALUE_NAME_STRING_dooms__Component \
TYPE_CHECK_FUNCTION_Component \
private:


//Type Short Name ( without namespace, only type name ) Version Macros.
#define GENERATE_BODY_Component(...) GENERATE_BODY_FULLNAME_dooms__Component(__VA_ARGS__)


#undef GENERATE_BODY
#define GENERATE_BODY(...) GENERATE_BODY_FULLNAME_dooms__Component(__VA_ARGS__)


//-------------------------------------------


