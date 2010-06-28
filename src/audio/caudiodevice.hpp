#ifndef EE_AUDIOCAUDIODEVICE_H
#define EE_AUDIOCAUDIODEVICE_H

#include "caudiolistener.hpp"

namespace EE { namespace Audio {

class cAudioDevice {
	public :
		static cAudioDevice& GetInstance();
		static cAudioDevice& instance();
		static void AddReference();
		static void RemoveReference();

		ALCdevice* GetDevice() const;
		ALenum GetFormatFromChannelsCount(unsigned int ChannelsCount) const;
		
		bool isCreated();
	private :
		cAudioDevice();
		~cAudioDevice();

		static cAudioDevice* ourInstance;

		ALCdevice*   myDevice;
		ALCcontext*  myContext;
		unsigned int myRefCount;
};

}}

#endif