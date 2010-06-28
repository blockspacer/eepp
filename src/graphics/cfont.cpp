#include "cfont.hpp"

namespace EE { namespace Graphics {

cFont::cFont() :
	mCacheData(true),
	mColor(0xFFFFFFFF),
	mShadowColor(0xFF000000),
	mNumLines(1),
	mTexId(0),
	mCachedWidth(0.f),
	mHeight(0),
	mSize(0)
{
}

cFont::~cFont() {
	mText.clear();
	mGlyphs.clear();
}

void cFont::SetText( const std::wstring& Text ) {
	if ( mText.size() != Text.size() ) {
		mRenderCoords.resize( Text.size() * 4 );
		mColors.resize( Text.size() * 4, mColor );
	}

	mText = Text;

	if ( mCacheData )
		CacheWidth();
}

void cFont::SetText( const std::string& Text) {
	SetText ( stringTowstring(Text) );
}

const eeColorA& cFont::Color() const {
	return mColor;
}

void cFont::Color(const eeColorA& Color) {
	if ( mColor != Color ) {
		mColor = Color;

		mColors.assign( mText.size() * 4, mColor );
	}
}

const eeColorA& cFont::ShadowColor() const {
	return mShadowColor;
}

void cFont::ShadowColor(const eeColorA& Color) {
	mShadowColor = Color;
}

void cFont::CacheNumLines() {
	Int32 c = 1;

	for ( eeUint i = 0; i < mText.size(); i++ ) {
		if ( mText[i] == L'\n' )
			c++;
	}

	mNumLines = c;
}

eeInt cFont::GetNumLines() {
	return mNumLines;
}

eeFloat cFont::GetTextWidth( const std::wstring& Text ) {
	SetText( Text );
	return mCachedWidth;
}

eeFloat cFont::GetTextWidth() const {
	return mCachedWidth;
}

Uint32 cFont::GetFontSize() const {
	return mSize;
}

Uint32 cFont::GetFontHeight() const {
	return mHeight;
}

std::wstring cFont::GetText() {
	return mText;
}

void cFont::CacheData( bool Cache ) {
	mCacheData = Cache;
}

const bool& cFont::CacheData() const {
	return mCacheData;
}

eeFloat cFont::GetTextHeight() {
	return (eeFloat)GetFontSize() * (eeFloat)GetNumLines();
}

const std::vector<eeFloat>& cFont::GetLinesWidth() const {
	return mLinesWidth;
}

void cFont::Draw( const std::string& Text, const eeFloat& X, const eeFloat& Y, const Uint32& Flags, const eeFloat& Scale, const eeFloat& Angle, const EE_RENDERALPHAS& Effect ) {
	Draw( stringTowstring(Text), X, Y, Flags, Scale, Angle, Effect );
}

void cFont::Draw( const eeFloat& X, const eeFloat& Y, const Uint32& Flags, const eeFloat& Scale, const eeFloat& Angle, const EE_RENDERALPHAS& Effect) {
	SubDraw( mText, X, Y, Flags, Scale, Angle, true, Effect );
}

void cFont::Draw( const std::wstring& Text, const eeFloat& X, const eeFloat& Y, const Uint32& Flags, const eeFloat& Scale, const eeFloat& Angle, const EE_RENDERALPHAS& Effect ) {
	SubDraw( Text, X, Y, Flags, Scale, Angle, false, Effect );
}

void cFont::Draw( cTextCache& TextCache, const eeFloat& X, const eeFloat& Y, const Uint32& Flags, const eeFloat& Scale, const eeFloat& Angle, const EE_RENDERALPHAS& Effect ) {
	if ( !TextCache.Text().size() )
		return;

	cTextureFactory::instance()->Bind( mTexId );
	cTextureFactory::instance()->SetBlendFunc( Effect );

	if ( Flags & FONT_DRAW_SHADOW ) {
		Uint32 f = Flags;
		f &= ~FONT_DRAW_SHADOW;

		eeColorA Col = mColor;

		//! FIXME: Cache shadows?
		SetText( TextCache.Text() );
		Color( mShadowColor );
		Draw( TextCache, X + 1, Y + 1, f, Scale, Angle, Effect );
		Color( Col );
	}

	eeFloat cX = (eeFloat) ( (Int32)X );
	eeFloat cY = (eeFloat) ( (Int32)Y );
	eeFloat nX = 0;
	eeFloat nY = 0;
	Int16 Char = 0;
	eeUint Line = 0;
	eeUint numvert = 0;

	if ( Angle != 0.0f || Scale != 1.0f ) {
		glLoadIdentity();
		glPushMatrix();

		eeVector2f Center( cX + TextCache.GetTextWidth() * 0.5f, cY + TextCache.GetTextHeight() * 0.5f );
		glTranslatef( Center.x , Center.y, 0.f );
		glRotatef( Angle, 0.0f, 0.0f, 1.0f );
		glScalef( Scale, Scale, 1.0f );
		glTranslatef( -Center.x, -Center.y, 0.f );
	}

	std::vector<eeVertexCoords>& RenderCoords = TextCache.VertextCoords();
	std::vector<eeColorA>& Colors = TextCache.Colors();

	switch ( FontHAlignGet( Flags ) ) {
		case FONT_DRAW_CENTER:
			nX = (eeFloat)( (Int32)( ( TextCache.GetTextWidth() - TextCache.LinesWidth()[ Line ] ) * 0.5f ) );
			Line++;
			break;
		case FONT_DRAW_RIGHT:
			nX = TextCache.GetTextWidth() - TextCache.LinesWidth()[ Line ];
			Line++;
			break;
	}

	for ( eeUint i = 0; i < TextCache.Text().size(); i++ ) {
		Char = static_cast<Int32>( TextCache.Text().at(i) );
		eeTexCoords* C = &mTexCoords[ Char ];

		switch(Char) {
			case L'\v':
				if (mVerticalDraw)
					nY += GetFontHeight();
				else
					nX += mGlyphs[ Char ].Advance;
				break;
			case L'\t':
				if (mVerticalDraw)
					nY += GetFontHeight() * 4;
				else
					nX += mGlyphs[ Char ].Advance * 4;
				break;
			case L'\n':
				if (mVerticalDraw) {
					nX += (GetFontSize() * Scale);
					nY = 0;
				} else {
					if ( i + 1 < TextCache.Text().size() ) {
						switch ( FontHAlignGet( Flags ) ) {
							case FONT_DRAW_CENTER:
								nX = (eeFloat)( (Int32)( ( TextCache.GetTextWidth() - TextCache.LinesWidth()[ Line ] ) * 0.5f ) );
								break;
							case FONT_DRAW_RIGHT:
								nX = TextCache.GetTextWidth() - TextCache.LinesWidth()[ Line ];
								break;
							default:
								nX = 0;
						}
					}

					nY += (GetFontSize() * Scale);
					Line++;
				}

				break;
			default:
				for ( Uint8 z = 0; z < 8; z+=2 ) {
					RenderCoords[ numvert ].TexCoords[0] = C->TexCoords[z];
					RenderCoords[ numvert ].TexCoords[1] = C->TexCoords[ z + 1 ];
					RenderCoords[ numvert ].Vertex[0] = cX + C->Vertex[z] + nX;
					RenderCoords[ numvert ].Vertex[1] = cY + C->Vertex[ z + 1 ] + nY;
					numvert++;
				}

				if (mVerticalDraw)
					nY += GetFontHeight();
				else
					nX += mGlyphs[ Char ].Advance;
		}
	}

	glColorPointer( 4, GL_UNSIGNED_BYTE, 0, reinterpret_cast<char*>( &Colors[0] ) );
	glTexCoordPointer( 2, GL_FLOAT, sizeof(eeVertexCoords), reinterpret_cast<char*>( &RenderCoords[0] ) );
	glVertexPointer( 2, GL_FLOAT, sizeof(eeVertexCoords), reinterpret_cast<char*>( &RenderCoords[0] ) + sizeof(eeFloat) * 2 );

	glDrawArrays( GL_QUADS, 0, numvert );

	if ( Angle != 0.0f || Scale != 1.0f )
		glPopMatrix();
}

void cFont::SubDraw( const std::wstring& Text, const eeFloat& X, const eeFloat& Y, const Uint32& Flags, const eeFloat& Scale, const eeFloat& Angle, const bool& Cached, const EE_RENDERALPHAS& Effect ) {
	if ( !Text.size() )
		return;

	cTextureFactory::instance()->Bind( mTexId );
	cTextureFactory::instance()->SetBlendFunc( Effect );

	if ( !Cached && ( Text.size() != mRenderCoords.size() / 4 || Angle != 0.f || Scale != 1.f || FontHAlignGet( Flags ) == FONT_DRAW_CENTER || FontHAlignGet( Flags ) == FONT_DRAW_RIGHT ) ) {
		SetText( Text );
	}

	if ( Flags & FONT_DRAW_SHADOW ) {
		Uint32 f = Flags;
		f &= ~FONT_DRAW_SHADOW;

		eeColorA Col = mColor;

		Color( mShadowColor );
		SubDraw( Text, X + 1, Y + 1, f, Scale, Angle, true, Effect );
		Color( Col );
	}

	eeFloat cX = (eeFloat) ( (Int32)X );
	eeFloat cY = (eeFloat) ( (Int32)Y );
	eeFloat nX = 0;
	eeFloat nY = 0;
	Int16 Char = 0;
	eeUint Line = 0;
	eeUint numvert = 0;

	if ( Angle != 0.0f || Scale != 1.0f ) {
		glLoadIdentity();
		glPushMatrix();

		eeVector2f Center( cX + GetTextWidth() * 0.5f, cY + GetTextHeight() * 0.5f );
		glTranslatef( Center.x , Center.y, 0.f );
		glRotatef( Angle, 0.0f, 0.0f, 1.0f );
		glScalef( Scale, Scale, 1.0f );
		glTranslatef( -Center.x, -Center.y, 0.f );
	}

	switch ( FontHAlignGet( Flags ) ) {
		case FONT_DRAW_CENTER:
			nX = (eeFloat)( (Int32)( ( mCachedWidth - mLinesWidth[ Line ] ) * 0.5f ) );
			Line++;
			break;
		case FONT_DRAW_RIGHT:
			nX = mCachedWidth - mLinesWidth[ Line ];
			Line++;
			break;
	}

	for ( eeUint i = 0; i < Text.size(); i++ ) {
		Char = static_cast<Int32>( Text.at(i) );
		eeTexCoords* C = &mTexCoords[ Char ];

		switch(Char) {
			case L'\v':
				if (mVerticalDraw)
					nY += GetFontHeight();
				else
					nX += mGlyphs[ Char ].Advance;
				break;
			case L'\t':
				if (mVerticalDraw)
					nY += GetFontHeight() * 4;
				else
					nX += mGlyphs[ Char ].Advance * 4;
				break;
			case L'\n':
				if (mVerticalDraw) {
					nX += (GetFontSize() * Scale);
					nY = 0;
				} else {
					if ( i + 1 < Text.size() ) {
						switch ( FontHAlignGet( Flags ) ) {
							case FONT_DRAW_CENTER:
								nX = (eeFloat)( (Int32)( ( mCachedWidth - mLinesWidth[ Line ] ) * 0.5f ) );
								break;
							case FONT_DRAW_RIGHT:
								nX = mCachedWidth - mLinesWidth[ Line ];
								break;
							default:
								nX = 0;
						}
					}

					nY += (GetFontSize() * Scale);
					Line++;
				}

				break;
			default:
				for ( Uint8 z = 0; z < 8; z+=2 ) {
					mRenderCoords[ numvert ].TexCoords[0] = C->TexCoords[z];
					mRenderCoords[ numvert ].TexCoords[1] = C->TexCoords[ z + 1 ];
					mRenderCoords[ numvert ].Vertex[0] = cX + C->Vertex[z] + nX;
					mRenderCoords[ numvert ].Vertex[1] = cY + C->Vertex[ z + 1 ] + nY;
					numvert++;
				}

				if (mVerticalDraw)
					nY += GetFontHeight();
				else
					nX += mGlyphs[ Char ].Advance;
		}
	}

	glColorPointer( 4, GL_UNSIGNED_BYTE, 0, reinterpret_cast<char*>( &mColors[0] ) );
	glTexCoordPointer( 2, GL_FLOAT, sizeof(eeVertexCoords), reinterpret_cast<char*>( &mRenderCoords[0] ) );
	glVertexPointer( 2, GL_FLOAT, sizeof(eeVertexCoords), reinterpret_cast<char*>( &mRenderCoords[0] ) + sizeof(eeFloat) * 2 );

	glDrawArrays( GL_QUADS, 0, numvert );

	if ( Angle != 0.0f || Scale != 1.0f )
		glPopMatrix();
}

void cFont::CacheWidth( const std::wstring& Text, std::vector<eeFloat>& LinesWidth, eeFloat& CachedWidth, eeInt& NumLines ) {
	LinesWidth.clear();

	eeFloat Width = 0, MaxWidth = 0;
	Uint16 CharID;
	Int32 Lines = 1;

	for (std::size_t i = 0; i < Text.size(); ++i) {
		CharID = static_cast<Uint16>( Text.at(i) );

		Width += mGlyphs[CharID].Advance;

		if ( CharID == L'\t' )
			Width += mGlyphs[CharID].Advance * 3;

		if ( CharID == L'\n' ) {
			Lines++;
			eeFloat lWidth = ( CharID == L'\t' ) ? mGlyphs[CharID].Advance * 4.f : mGlyphs[CharID].Advance;
			LinesWidth.push_back( Width - lWidth );
			Width = 0;
		}

		if ( Width > MaxWidth )
			MaxWidth = Width;
	}

	if ( Text.size() && Text.at( Text.size() - 1 ) != '\n' ) {
		LinesWidth.push_back( Width );
	}

	CachedWidth = MaxWidth;
	NumLines = Lines;
}

void cFont::CacheWidth() {
	CacheWidth( mText, mLinesWidth, mCachedWidth, mNumLines );
}

void cFont::ShrinkText( std::string& Str, const Uint32& MaxWidth ) {
	if ( !Str.size() )
		return;

	eeFloat		tCurWidth		= 0.f;
	eeFloat 	tWordWidth		= 0.f;
	eeFloat 	tMaxWidth		= (eeFloat) MaxWidth;
	Int32 		tPrev			= -1;
	char *		tStringLoop		= &Str[0];
	char *		tLastSpace		= NULL;

	while ( *tStringLoop ) {
		if ( (Uint32)( *tStringLoop ) < mGlyphs.size() ) {
			eeGlyph * pChar = &mGlyphs[ ( *tStringLoop ) ];
			eeFloat fCharWidth	= (eeFloat)pChar->Advance;

			if ( ( *tStringLoop ) == '\t' )
				fCharWidth += pChar->Advance * 3;

			tWordWidth		+= fCharWidth;

			if ( ' ' == *tStringLoop || '\0' == *( tStringLoop + 1 ) ) {
				if ( tCurWidth + tWordWidth < tMaxWidth ) {
					tCurWidth		+= tWordWidth;
					tLastSpace		= tStringLoop;

					tPrev			= *tStringLoop;

					tStringLoop++;
				} else {
					if ( NULL != tLastSpace ) {
						*tLastSpace		= '\n';
						tStringLoop	= tLastSpace + 1;
					} else {
						*tStringLoop	= '\n';
					}

					tLastSpace		= NULL;
					tCurWidth		= 0.f;
					tPrev			= -1;
				}

				tWordWidth = 0.f;
			} else if ( '\n' == *tStringLoop ) {
				tWordWidth 		= 0.f;
				tCurWidth 		= 0.f;
				tPrev			= ( *tStringLoop );
				tLastSpace		= NULL;
				tStringLoop++;
			} else {
				tPrev			= ( *tStringLoop );
				tStringLoop++;
			}
		} else {
			*tStringLoop		= ' ';
			tPrev				= ' ';
		}
	}
}

void cFont::ShrinkText( std::wstring& Str, const Uint32& MaxWidth ) {
	if ( !Str.size() )
		return;

	eeFloat		tCurWidth		= 0.f;
	eeFloat 	tWordWidth		= 0.f;
	eeFloat 	tMaxWidth		= (eeFloat) MaxWidth;
	Int32 		tPrev			= -1;
	wchar_t *	tStringLoop		= &Str[0];
	wchar_t	*	tLastSpace		= NULL;

	while ( *tStringLoop ) {
		if ( (Uint32)( *tStringLoop ) < mGlyphs.size() ) {
			eeGlyph * pChar = &mGlyphs[ ( *tStringLoop ) ];
			eeFloat fCharWidth	= (eeFloat)pChar->Advance;

			if ( ( *tStringLoop ) == L'\t' )
				fCharWidth += pChar->Advance * 3;

			// Add the new char width to the current word width
			tWordWidth		+= fCharWidth;

			if ( L' ' == *tStringLoop || L'\0' == *( tStringLoop + 1 ) ) {

				// If current width plus word width is minor to the max width, continue adding
				if ( tCurWidth + tWordWidth < tMaxWidth ) {
					tCurWidth		+= tWordWidth;
					tLastSpace		= tStringLoop;

					tPrev			= *tStringLoop;

					tStringLoop++;
				} else {
					// If it was an space before, replace that space for an new line
					// Start counting from the new line first character
					if ( NULL != tLastSpace ) {
						*tLastSpace		= L'\n';
						tStringLoop	= tLastSpace + 1;
					} else {	// The word is larger than the current possible width
						*tStringLoop	= L'\n';
					}

					// Set the last spaces as null, because is a new line
					tLastSpace		= NULL;

					// New line, new current width
					tCurWidth		= 0.f;

					// No previous char for the new line
					tPrev			= -1;
				}

				// New word, so we reset the current word width
				tWordWidth = 0.f;
			} else if ( L'\n' == *tStringLoop ) {
				tWordWidth 		= 0.f;
				tCurWidth 		= 0.f;
				tPrev			= ( *tStringLoop );
				tLastSpace		= NULL;
				tStringLoop++;
			} else {
				tPrev			= ( *tStringLoop );
				tStringLoop++;
			}
		} else {	// Replace any unknown char as spaces.
			*tStringLoop		= L' ';
			tPrev				= L' ';
		}
	}
}

const Uint32& cFont::GetTexId() const {
	return mTexId;
}

}}