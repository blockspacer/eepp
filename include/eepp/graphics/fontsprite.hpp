#ifndef EE_GRAPHICS_FONTSPRITE_HPP
#define EE_GRAPHICS_FONTSPRITE_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/graphics/texture.hpp>

namespace EE { namespace System {
class Pack;
class IOStream;
}} // namespace EE::System

namespace EE { namespace Graphics {

/** @brief Implementation of XNA Font Sprites */
class EE_API FontSprite : public Font {
  public:
	static FontSprite* New( const std::string FontName );

	static FontSprite* New( const std::string FontName, const std::string& filename );

	~FontSprite();

	bool loadFromFile( const std::string& filename, Color key = Color::Fuchsia,
					   Uint32 firstChar = 32, int spacing = 0 );

	bool loadFromMemory( const void* data, std::size_t sizeInBytes, Color key = Color::Fuchsia,
						 Uint32 firstChar = 32, int spacing = 0 );

	bool loadFromStream( IOStream& stream, Color key = Color::Fuchsia, Uint32 firstChar = 32,
						 int spacing = 0 );

	bool loadFromPack( Pack* pack, std::string filePackPath, Color key = Color::Fuchsia,
					   Uint32 firstChar = 32, int spacing = 0 );

	const Font::Info& getInfo() const;

	const Glyph& getGlyph( Uint32 codePoint, unsigned int characterSize, bool bold,
						   Float outlineThickness = 0 ) const;

	Float getKerning( Uint32 first, Uint32 second, unsigned int characterSize ) const;

	Float getLineSpacing( unsigned int characterSize ) const;

	Uint32 getFontHeight( const Uint32& characterSize );

	Float getUnderlinePosition( unsigned int characterSize ) const;

	Float getUnderlineThickness( unsigned int characterSize ) const;

	Texture* getTexture( unsigned int characterSize ) const;

	bool loaded() const;

	FontSprite& operator=( const FontSprite& right );

  protected:
	FontSprite( const std::string FontName );

	typedef std::map<Uint64, Glyph> GlyphTable; ///< Table mapping a codepoint to its glyph

	struct Page {
		GlyphTable glyphs; ///< Table mapping code points to their corresponding glyph
		Texture* texture;  ///< Texture containing the pixels of the glyphs
	};

	void cleanup();

	typedef std::map<unsigned int, Page>
		PageTable; ///< Table mapping a character size to its page (texture)

	Font::Info mInfo;		  ///< Information about the font
	mutable PageTable mPages; ///< Table containing the glyphs pages by character size
	std::string mFilePath;
	Uint32 mFontSize;

	Glyph loadGlyph( Uint32 codePoint, unsigned int characterSize, bool bold,
					 Float outlineThickness ) const;
};

}} // namespace EE::Graphics

#endif
