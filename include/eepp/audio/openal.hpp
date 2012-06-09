#ifndef EE_OPENAL_H
#define EE_OPENAL_H

#include <eepp/audio/base.hpp>

#if EE_PLATFORM == EE_PLATFORM_MACOSX || EE_PLATFORM == EE_PLATFORM_IOS
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

namespace EE { namespace Audio {

#ifdef EE_DEBUG
	// If in debug mode, perform a test on every call
	#define ALCheck(Func) ((Func), ALCheckError(__FILE__, __LINE__))
#else
	#define ALCheck(Func) (Func)
#endif

void ALCheckError(const std::string& File, unsigned int Line);

void EnsureALInit();

}}
#endif