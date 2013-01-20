#ifndef EE_GRAPHICSCTEXTUREFONTLOADER
#define EE_GRAPHICSCTEXTUREFONTLOADER

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/cfont.hpp>
#include <eepp/graphics/ctexturefont.hpp>
#include <eepp/system/cobjectloader.hpp>
#include <eepp/graphics/ctextureloader.hpp>

namespace EE { namespace Graphics {

/** @brief The Texture Font loads a texture font in synchronous or asynchronous mode.
@see cObjectLoader */
class EE_API cTextureFontLoader : public cObjectLoader {
	public:
		/** Loads a texture font from a texture ( only for monospaced fonts )
		*	@param FontName The font name
		*	@param TexLoader An instance of a texture loader that will be used to load the texture. The instance will be released by the Texture Font Loader when is destroyed.
		*	@param StartChar The fist char represented on the texture
		*	@param Spacing The space between every char ( default 0 means TextureWidth / TexColumns )
		*	@param VerticalDraw If true render the string verticaly
		*	@param TexColumns The number of chars per column
		*	@param TexRows The number of chars per row
		*	@param NumChars The number of characters to read from the texture
		*/
		cTextureFontLoader( const std::string FontName, cTextureLoader * TexLoader, const eeUint& StartChar = 0, const eeUint& Spacing = 0, const bool& VerticalDraw = false, const eeUint& TexColumns = 16, const eeUint& TexRows = 16, const Uint16& NumChars = 256 );

		/** Load's a texture font and then load's the character coordinates file ( generated by the cTTFFont class )
		*	@param FontName The font name
		*	@param TexLoader An instance of a texture loader that will be used to load the texture. The instance will be released by the Texture Font Loader when is destroyed.
		*	@param CoordinatesDatPath The character coordinates file ( this is the file created when the TTF font was converted to a texture font. @see cTTFFont::Save ).
		*	@param VerticalDraw If true render the string verticaly
		*/
		cTextureFontLoader( const std::string FontName, cTextureLoader * TexLoader, const std::string& CoordinatesDatPath, const bool& VerticalDraw = false );

		/** Load's a texture font and then load's the character coordinates file ( generated by the cTTFFont class ) stored in a pack file.
		*	@param FontName The font name
		*	@param TexLoader An instance of a texture loader that will be used to load the texture. The instance will be released by the Texture Font Loader when is destroyed.
		*	@param Pack The pack used to load the characters coordinates
		*	@param FilePackPath The character coordinates file path inside the pack ( this is the file created when the TTF font was converted to a texture font. @see cTTFFont::Save ).
		*	@param VerticalDraw If true render the string verticaly
		*/
		cTextureFontLoader( const std::string FontName, cTextureLoader * TexLoader, cPack * Pack, const std::string& FilePackPath, const bool& VerticalDraw = false );

		/** Load's a texture font and then load's the character coordinates file ( generated by the cTTFFont class ) from memory.
		*	@param FontName The font name
		*	@param TexLoader An instance of a texture loader that will be used to load the texture. The instance will be released by the Texture Font Loader when is destroyed.
		*	@param CoordData The character coordinates buffer pointer ( this is the file created when the TTF font was converted to a texture font. @see cTTFFont::Save ).
		*	@param CoordDataSize The buffer pointer size
		*	@param VerticalDraw If true render the string verticaly
		*/
		cTextureFontLoader( const std::string FontName, cTextureLoader * TexLoader, const char* CoordData, const Uint32& CoordDataSize, const bool& VerticalDraw = false );

		virtual ~cTextureFontLoader();

		/** Updates the current state of the loading in progress ( must be called from the instancer thread, usually the main thread ).
		* @see cObjectLoader::Update */
		void 				Update();

		/** @brief Releases the font loaded ( if was already loaded ) */
		void				Unload();

		/** @return The font name */
		const std::string&	Id() const;

		/** @return The instance of the font created after loading it. ( NULL if was not created yet ) */
		cFont *				Font() const;
	protected:
		enum TEXTURE_FONT_LOAD_TYPE
		{
			TEF_LT_PATH	= 1,
			TEF_LT_MEM	= 2,
			TEF_LT_PACK	= 3,
			TEF_LT_TEX	= 4
		};

		Uint32				mLoadType; 	// From memory, from path, from pack

		cTextureFont *		mFont;

		std::string			mFontName;
		cTextureLoader *	mTexLoader;

		std::string			mFilepath;

		eeUint				mStartChar;
		eeUint				mSpacing;
		bool				mVerticalDraw;
		eeUint				mTexColumns;
		eeUint				mTexRows;
		eeUint				mNumChars;

		cPack *				mPack;

		const char *		mData;
		Uint32				mDataSize;

		void 				Start();

		void				Reset();
	private:
		bool				mTexLoaded;
		bool				mFontLoaded;

		void				LoadFont();
		void 				LoadFromPath();
		void				LoadFromMemory();
		void				LoadFromPack();
		void				LoadFromTex();
};

}}

#endif


