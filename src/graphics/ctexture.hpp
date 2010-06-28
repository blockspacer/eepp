#ifndef EE_GRAPHICSCTEXTURE_H
#define EE_GRAPHICSCTEXTURE_H

#include "base.hpp"

namespace EE { namespace Graphics {

class EE_API cTexture {
	public:
		cTexture();
		~cTexture();

		cTexture( const Uint32& texture, const eeInt& width, const eeInt& height, const eeInt& imgwidth, const eeInt& imgheight, const bool& UseMipmap, const eeUint& Channels, const std::string& filepath, const eeRGB& ColorKey, const EE_CLAMP_MODE& ClampMode, const bool& CompressedTexture, const Uint8* data = NULL );
		cTexture( const cTexture& Copy );
		cTexture& operator =(const cTexture& Other);

		/** Create the Texture
		* @param texture The OpenGL Texture Id (texture handler)
		* @param width The Texture Width
		* @param height The Texture Height
		* @param imgwidth The Image Width
		* @param imgheight The Image Height
		* @param UseMipmap Set if Texture has Mipmaps
		* @param Channels The Texture number of channels.
		* @param filepath The Texture File path
		* @param ColorKey The Texture Color Key
		* @param ClampMode The Texture Clamp Mode
		* @param CompressedTexture The Texture was compressed on loading
		* @param data The Texture (raw texture)
		*/
		void Create( const Uint32& texture, const eeInt& width, const eeInt& height, const eeInt& imgwidth, const eeInt& imgheight, const bool& UseMipmap, const eeUint& Channels, const std::string& filepath, const eeRGB& ColorKey, const EE_CLAMP_MODE& ClampMode, const bool& CompressedTexture, const Uint8* data = NULL );

		/** Set the OpenGL Texture Id (texture handler) */
		void Texture( const Uint32& texture ) { mTexture = texture; }

		/** @return The OpenGL Texture Id (texture handler) */
		Uint32 Texture() const { return mTexture; }

		/** @return The Texture Size on Memory (in bytes) */
		eeUint Size() const { return (eeUint)mWidth * (eeUint)mHeight * 4; }

		/** Set the Texture File Path */
		void Filepath( const std::string& filepath ) { mFilepath = filepath; }

		/** @return The Texture File Path */
		std::string Filepath() const { return mFilepath; }

		/** Set the Texture Width */
		void Width( const eeInt& width ) { mWidth = width; }

		/** @return The Texture Width */
		eeInt Width() const { return mWidth; }

		/** Set the Texture Height */
		void Height( const eeInt& height ) { mHeight = height; }

		/** @return The Texture Height */
		eeInt Height() const { return mHeight; }

		/** @return The Image Width */
		eeInt ImgWidth() const { return mImgWidth; }

		/** @return The Image Height */
		eeInt ImgHeight() const { return mImgHeight; }

		/** Set if the Texture use Mipmaps */
		void Mipmap( const bool& UseMipmap ) { mMipmap = UseMipmap; }

		/** @return If the texture use Mipmaps */
		bool Mipmap() const { return mMipmap; }

		/** Set the Texture Color Key */
		void ColorKey( const eeRGB& colorkey ) { mColorKey = colorkey; }

		/** @return The Texture Color Key */
		eeRGB ColorKey() const { return mColorKey; }

		/** Set the Texture Clamp Mode */
		void ClampMode( const EE_CLAMP_MODE& clampmode );

		/** @return The Texture Clamp Mode */
		EE_CLAMP_MODE ClampMode() const { return mClampMode; }

		/** Lock the Texture for direct access */
		eeColorA* Lock();

		/** Return the pixel color from the texture. \n You must have a copy of the texture on local memory. For that you need to Lock the texture first. */
		const eeColorA& GetPixel(const eeUint& x, const eeUint& y);

		/** Set the pixel color to the texture. \n You must have a copy of the texture on local memory. For that you need to Lock the texture first. */
		void SetPixel(const eeUint& x, const eeUint& y, const eeColorA& Color);

		/** Unlock the previously locked Texture */
		bool Unlock(const bool& KeepData = false, const bool& Modified = false);

		/** @return A pointer to the first pixel of the texture ( keeped with a local copy ). \n You must have a copy of the texture on local memory. For that you need to Lock the texture first. */
		const Uint8* GetPixelsPtr();

		/** Assign a new array of pixels to the texture in local memory ( it has to be exactly of the same size of the texture ) */
		void Pixels( const Uint8* data );

		/** Set the Texture Filter Mode */
		void SetTextureFilter(const EE_TEX_FILTER& filter);

		/** Save the Texture to a new File */
		bool SaveToFile(const std::string& filepath, const EE_SAVETYPE& Format);

		/** Create an Alpha mask from a Color */
		void CreateMaskFromColor(eeColorA ColorKey, Uint8 Alpha);

		/** Create an Alpha mask from a Color */
		void CreateMaskFromColor(eeColor ColorKey, Uint8 Alpha);

		/** Replace a color on the texture */
		void ReplaceColor(eeColorA ColorKey, eeColorA NewColor);

		/** @return If the Texture has a copy on the local memory */
		bool LocalCopy();

		/** Unload the Texture from Memory */
		void DeleteTexture();

		/** Set if the Texture is Grabed */
		void Grabed( const bool& isGrabed ) { mGrabed = isGrabed; }

		/** @return If the texture is Grabed */
		bool Grabed() const { return mGrabed; }
		
		/** @return The current texture filter */
		EE_TEX_FILTER Filter() const { return mFilter; }

		/** @return If the texture was compressed on load (DXT compression) */
		bool Compressed() const { return mCompressedTexture; };
		
		/** @return The number of channels used by the image */
		eeUint Channels() const { return mChannels; }

		/** Clears the current texture cache if exists */
		void ClearCache();
	protected:
		std::string mFilepath;
		
		Uint32 mId;
		
		GLint mTexture;
		
		eeInt mWidth;
		eeInt mHeight;
		eeInt mImgWidth;
		eeInt mImgHeight;
		
		eeUint mChannels;
		
		bool mMipmap;
		bool mModified;
		eeRGB mColorKey;
		
		EE_CLAMP_MODE mClampMode;
		EE_TEX_FILTER mFilter;
		bool mCompressedTexture;
		
		bool mLocked;
		bool mGrabed;
		
		eeColorA* mPixels;
		
		void ApplyClampMode();
};

}}

#endif