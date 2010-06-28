#ifndef EE_AUDIOCSOUNDFILEOGG_H
#define EE_AUDIOCSOUNDFILEOGG_H

#include "base.hpp"
#include "csoundfile.hpp"

namespace EE { namespace Audio {

class cSoundFileOgg : public cSoundFile {
	public:
		cSoundFileOgg();
		~cSoundFileOgg();
		
		/** Check if a given file is supported by this loader. */
		static bool IsFileSupported(const std::string& Filename, bool Read);
		
		/** Check if a given file in memory is supported by this loader. */
		static bool IsFileSupported(const char* Data, std::size_t SizeInBytes);
	
		virtual std::size_t Read(Int16* Data, std::size_t NbSamples);
	private :
		virtual bool OpenRead(const std::string& Filename, std::size_t& NbSamples, unsigned int& ChannelsCount, unsigned int& SampleRate);
		virtual bool OpenRead(const char* Data, std::size_t SizeInBytes, std::size_t& NbSamples, unsigned int& ChannelsCount, unsigned int& SampleRate);
	
		stb_vorbis*  myStream;		///< Vorbis stream
		unsigned int myChannelsCount; ///< Number of channels (1 = mono, 2 = stereo)
};

}}

#endif