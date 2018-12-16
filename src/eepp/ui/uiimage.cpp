#include <eepp/ui/uiimage.hpp>
#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/sprite.hpp>
#include <eepp/graphics/drawablesearcher.hpp>
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI {

UIImage * UIImage::New() {
	return eeNew( UIImage, () );
}

UIImage::UIImage() :
	UIWidget(),
	mScaleType( UIScaleType::None ),
	mDrawable( NULL ),
	mColor(),
	mAlignOffset(0,0),
	mDrawableOwner(false)
{
	mFlags |= UI_AUTO_SIZE;

	onAutoSize();
}

UIImage::~UIImage() {
	safeDeleteDrawable();
}

Uint32 UIImage::getType() const {
	return UI_TYPE_IMAGE;
}

bool UIImage::isType( const Uint32& type ) const {
	return UIImage::getType() == type ? true : UIWidget::isType( type );
}

UIImage * UIImage::setDrawable( Drawable * drawable ) {
	safeDeleteDrawable();

	mDrawable = drawable;

	onAutoSize();

	if ( NULL != mDrawable && mDpSize.x == 0 && mDpSize.y == 0 ) {
		setSize( mDrawable->getSize() );
	}

	autoAlign();

	notifyLayoutAttrChange();

	invalidateDraw();

	return this;
}

void UIImage::onAutoSize() {
	if ( NULL != mDrawable ) {
		if ( ( mFlags & UI_AUTO_SIZE ) && Sizef::Zero == mDpSize ) {
			setSize( mDrawable->getSize() );
		}

		if ( mLayoutWidthRules == WRAP_CONTENT ) {
			setInternalPixelsWidth( (int)mDrawable->getSize().getWidth() + mRealPadding.Left + mRealPadding.Right );
		}

		if ( mLayoutHeightRules == WRAP_CONTENT ) {
			setInternalPixelsHeight( (int)mDrawable->getSize().getHeight() + mRealPadding.Top + mRealPadding.Bottom );
		}
	}
}

void UIImage::calcDestSize() {
	if ( mScaleType == UIScaleType::Expand ) {
		mDestSize = Sizef( mSize.x - mRealPadding.Left - mRealPadding.Right, mSize.y - mRealPadding.Top - mRealPadding.Bottom );
	} else if ( mScaleType == UIScaleType::FitInside ) {
		if ( NULL == mDrawable)
			return;

		Sizef pxSize( PixelDensity::dpToPx( mDrawable->getSize() ) );
		Float Scale1 = ( mSize.x - mRealPadding.Left - mRealPadding.Right ) / pxSize.x;
		Float Scale2 = ( mSize.y - mRealPadding.Top - mRealPadding.Bottom ) / pxSize.y;

		if ( Scale1 < 1 || Scale2 < 1 ) {
			if ( Scale2 < Scale1 )
				Scale1 = Scale2;

			mDestSize = Sizef( pxSize.x * Scale1, pxSize.y * Scale1 );
		} else {
			mDestSize = Sizef( pxSize.x, pxSize.y );
		}
	} else {
		if ( NULL == mDrawable)
			return;

		mDestSize = mDrawable->getSize();
	}

	mDestSize = mDestSize.floor();

	autoAlign();
}

void UIImage::draw() {
	UINode::draw();

	if ( mVisible ) {
		if ( NULL != mDrawable && 0.f != mAlpha ) {
			calcDestSize();

			mDrawable->setColor( mColor );
			mDrawable->draw( Vector2f( (Float)mScreenPosi.x + (int)mAlignOffset.x, (Float)mScreenPosi.y + (int)mAlignOffset.y ), mDestSize );
			mDrawable->clearColor();
		}
	}
}

void UIImage::setAlpha( const Float& alpha ) {
	UINode::setAlpha( alpha );
	mColor.a = (Uint8)alpha;
}

Drawable * UIImage::getDrawable() const {
	return mDrawable;
}

const Color& UIImage::getColor() const {
	return mColor;
}

UIImage * UIImage::setColor( const Color& col ) {
	mColor = col;
	setAlpha( col.a );
	return this;
}

void UIImage::autoAlign() {
	if ( NULL == mDrawable )
		return;

	if ( HAlignGet( mFlags ) == UI_HALIGN_CENTER ) {
		mAlignOffset.x = ( mSize.getWidth()- mDestSize.x ) / 2;
	} else if ( fontHAlignGet( mFlags ) == UI_HALIGN_RIGHT ) {
		mAlignOffset.x =  mSize.getWidth() - mDestSize.x - mRealPadding.Right;
	} else {
		mAlignOffset.x = mRealPadding.Left;
	}

	if ( VAlignGet( mFlags ) == UI_VALIGN_CENTER ) {
		mAlignOffset.y = ( mSize.getHeight() - mDestSize.y ) / 2;
	} else if ( fontVAlignGet( mFlags ) == UI_VALIGN_BOTTOM ) {
		mAlignOffset.y = mSize.getHeight() - mDestSize.y - mRealPadding.Bottom;
	} else {
		mAlignOffset.y = mRealPadding.Top;
	}
}

void UIImage::safeDeleteDrawable() {
	if ( NULL != mDrawable && mDrawableOwner ) {
		if ( mDrawable->getDrawableType() == Drawable::SPRITE ) {
			Sprite * spr = reinterpret_cast<Sprite*>( mDrawable );
			eeSAFE_DELETE( spr );
		}

		mDrawableOwner = false;
	}
}

void UIImage::onSizeChange() {
	onAutoSize();
	calcDestSize();
	UIWidget::onSizeChange();
}

void UIImage::onAlignChange() {
	UIWidget::onAlignChange();
	onAutoSize();
	calcDestSize();
}

const Vector2f& UIImage::getAlignOffset() const {
	return mAlignOffset;
}

bool UIImage::setAttribute( const NodeAttribute& attribute ) {
	const std::string& name = attribute.getName();

	if ( "src" == name ) {
		Drawable * res = NULL;

		if ( NULL != ( res = DrawableSearcher::searchByName( attribute.asString() ) ) ) {
			if ( res->getDrawableType() == Drawable::SPRITE )
				mDrawableOwner = true;

			setDrawable( res );
		}
	} else if ( "scaletype" == name ) {
		std::string val = attribute.asString();
		String::toLowerInPlace( val );

		if ( "expand" == val ) {
			setScaleType( UIScaleType::Expand );
		} else if ( "fit_inside" == val || "fitinside" == val ) {
			setScaleType( UIScaleType::FitInside );
		} else if ( "none" == val ) {
			setScaleType( UIScaleType::None );
		}
	} else if ( "tint" == name ) {
		setColor( Color::fromString( attribute.asString() ) );
	} else {
		return UIWidget::setAttribute( attribute );
	}

	return true;
}

Uint32 UIImage::getScaleType() const {
	return mScaleType;
}

UIImage * UIImage::setScaleType(const Uint32& scaleType) {
	mScaleType = scaleType;
	calcDestSize();
	invalidateDraw();
	return this;
}

}}
