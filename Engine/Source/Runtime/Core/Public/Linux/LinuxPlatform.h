#pragma once


namespace Nexus
{

	struct FLinuxPlatformTypes : public FGenericPlatformTypes
	{

	};

	using FPlatformTypes = FLinuxPlatformTypes;

}

/** Inline definitions. */
#if NEXUS_DEBUG
	#define FORCEINLINE inline
#else
	#define FORCEINLINE inline __attribute__ ((always_inline))
#endif

#define FORCENOINLINE __attribute__ ((noinline))

/** DLL definitions. */
#define DLLIMPORT __attribute__((visibility("default")))
#define DLLEXPORT __attribute__((visibility("default")))