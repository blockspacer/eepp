#ifndef EE_AUDIO_SOUNDFILEREADEROGG_HPP
#define EE_AUDIO_SOUNDFILEREADEROGG_HPP

#include <eepp/audio/soundfilereader.hpp>
#define OV_EXCLUDE_STATIC_CALLBACKS
#include <vorbis/vorbisfile.h>

namespace EE { namespace Audio { namespace Private {

/// \brief Implementation of sound file reader that handles OGG/Vorbis files
class SoundFileReaderOgg : public SoundFileReader {
  public:
	////////////////////////////////////////////////////////////
	/// \brief Check if this reader can handle a file given by an input stream
	///
	/// \param stream Source stream to check
	///
	/// \return True if the file is supported by this reader
	///
	////////////////////////////////////////////////////////////
	static bool check( IOStream& stream );

  public:
	SoundFileReaderOgg();

	~SoundFileReaderOgg();

	////////////////////////////////////////////////////////////
	/// \brief Open a sound file for reading
	///
	/// \param stream Source stream to read from
	/// \param info   Structure to fill with the properties of the loaded sound
	///
	/// \return True if the file was successfully opened
	///
	////////////////////////////////////////////////////////////
	virtual bool open( IOStream& stream, Info& info );

	////////////////////////////////////////////////////////////
	/// \brief Change the current read position to the given sample offset
	///
	/// The sample offset takes the channels into account.
	/// If you have a time offset instead, you can easily find
	/// the corresponding sample offset with the following formula:
	/// `timeInSeconds * sampleRate * channelCount`
	/// If the given offset exceeds to total number of samples,
	/// this function must jump to the end of the file.
	///
	/// \param sampleOffset Index of the sample to jump to, relative to the beginning
	///
	////////////////////////////////////////////////////////////
	virtual void seek( Uint64 sampleOffset );

	////////////////////////////////////////////////////////////
	/// \brief Read audio samples from the open file
	///
	/// \param samples  Pointer to the sample array to fill
	/// \param maxCount Maximum number of samples to read
	///
	/// \return Number of samples actually read (may be less than \a maxCount)
	///
	////////////////////////////////////////////////////////////
	virtual Uint64 read( Int16* samples, Uint64 maxCount );

  private:
	void close();

	OggVorbis_File mVorbis;		// ogg/vorbis file handle
	unsigned int mChannelCount; // number of channels of the open sound file
};

}}} // namespace EE::Audio::Private

#endif
