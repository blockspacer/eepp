#include "cuigfx.hpp"

namespace EE { namespace UI {

cUIGfx::cUIGfx( const cUIGfx::CreateParams& Params ) : 
	cUIControlAnim( Params ),
	mShape( Params.Shape ),
	mColor( Params.ShapeColor ),
	mRender( Params.ShapeRender )
{
	mType |= UI_TYPE_GET(UI_TYPE_GFX);
	
	if ( Flags() & UI_AUTO_SIZE || ( Params.Size.x == -1 && Params.Size.y == -1 ) )
		Size( mShape->Size() );
	
	if ( mColor.voidRGB ) {
		mColor.Alpha = (Uint8)mAlpha;
		mColor.voidRGB = false;
	}
}

cUIGfx::~cUIGfx() {
}

void cUIGfx::Draw() {
	cUIControlAnim::Draw();
	
	if ( mVisible ) {
		eeVector2i Pos = mPos;
		ControlToScreen( Pos );
		
		mShape->Draw( (eeFloat)Pos.x, (eeFloat)Pos.y, mColor, 0.f, 1.f, mBlend, mRender );
	}
}

void cUIGfx::Alpha( const eeFloat& alpha ) {
	cUIControlAnim::Alpha( alpha );
	mColor.Alpha = (Uint8)alpha;
}

cShape * cUIGfx::Shape() const {
	return mShape;
}

const eeRGBA& cUIGfx::Color() const {
	return mColor;
}

void cUIGfx::Color( const eeRGBA& color ) {
	mColor = color;
	Alpha( color.A() );
}

const EE_RENDERTYPE& cUIGfx::RenderType() const {
	return mRender;
}

void cUIGfx::RenderType( const EE_RENDERTYPE& render ) {
	mRender = render;
}

void cUIGfx::OnSizeChange() {
	if ( Flags() & UI_FIT_TO_CONTROL ) {
		mShape->DestWidth( (eeFloat)mSize.x );
		mShape->DestHeight( (eeFloat)mSize.y );
	}
}

}}