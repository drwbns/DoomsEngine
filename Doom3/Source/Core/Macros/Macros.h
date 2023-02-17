#pragma once

// Never include HEAVY header.
// SEPERATE codes included in Core.h with implemation codes( use MACRO_IMPLEMENTATION for this )
// Use Forward declaration.
// And Define implemations in MACRO_IMPLEMENTATION

#include "CompilerMacros.h"





#ifndef EXIT_PROGRAM
#define EXIT_PROGRAM exit(0);
#endif

#include "MacrosHelper.h"

#include "DllMarcos.h"
#include "Path.h"
#include "Log.h"
#include "Assert.h"
#include "Profiling.h"

#include "TypeDef.h"


#if defined(RELEASE_MODE)

#ifndef ASSUME_ZERO
#define ASSUME_ZERO __assume(0) // https://docs.microsoft.com/en-us/cpp/intrinsics/assume?view=msvc-160
#endif

#elif defined(DEBUG_MODE)

#ifndef ASSUME_ZERO
#define ASSUME_ZERO D_ASSERT(0) // https://docs.microsoft.com/en-us/cpp/intrinsics/assume?view=msvc-160
#endif


#endif