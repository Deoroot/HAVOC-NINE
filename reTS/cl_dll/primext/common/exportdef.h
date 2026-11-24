// exportdef.h - DLLEXPORT, the mark on the functions the engine finds in the module by name.
// H9: written new for reTS from the interface (the one macro the module uses), replacing PrimeXT's file,
// which had no licence notice. Same licence as the PrimeXT files it is built with.
#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
#	define DLLEXPORT __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
#	define DLLEXPORT __attribute__((visibility("default")))
#else
#	define DLLEXPORT
#endif
