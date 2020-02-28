#include <eepp/scene/scenenode.hpp>
#include <eepp/ui/uimenu.hpp>
#include <eepp/ui/uimenusubmenu.hpp>

namespace EE { namespace UI {

UIMenuSubMenu* UIMenuSubMenu::New() {
	return eeNew( UIMenuSubMenu, () );
}

UIMenuSubMenu::UIMenuSubMenu() :
	UIMenuItem( "menu::submenu" ),
	mSubMenu( NULL ),
	mArrow( NULL ),
	mMaxTime( Milliseconds( 200.f ) ),
	mCbId( 0 ),
	mCbId2( 0 ),
	mCurWait( NULL ) {
	mArrow = UINode::New();
	mArrow->setParent( this );
	mArrow->setFlags( UI_AUTO_SIZE );
	mArrow->setVisible( true );
	mArrow->setEnabled( false );

	applyDefaultTheme();
}

UIMenuSubMenu::~UIMenuSubMenu() {}

Uint32 UIMenuSubMenu::getType() const {
	return UI_TYPE_MENUSUBMENU;
}

bool UIMenuSubMenu::isType( const Uint32& type ) const {
	return UIMenuSubMenu::getType() == type ? true : UIMenuItem::isType( type );
}

void UIMenuSubMenu::setTheme( UITheme* Theme ) {
	UIMenuItem::setTheme( Theme );

	mArrow->setThemeSkin( "menuarrow" );
	mArrow->setSize( mArrow->getSkinSize() );

	onStateChange();

	onThemeLoaded();
}

void UIMenuSubMenu::onSizeChange() {
	UIMenuItem::onSizeChange();

	mArrow->setPosition( getParent()->getSize().getWidth() - mArrow->getSize().getWidth() -
							 PixelDensity::dpToPx( 1 ),
						 0 );
	mArrow->centerVertical();
}

void UIMenuSubMenu::onAlphaChange() {
	UIMenuItem::onAlphaChange();

	mArrow->setAlpha( mAlpha );
}

void UIMenuSubMenu::onStateChange() {
	UIMenuItem::onStateChange();

	onSizeChange();
}

void UIMenuSubMenu::setSubMenu( UIMenu* SubMenu ) {
	if ( NULL != mSubMenu && mSubMenu != SubMenu ) {
		mSubMenu->removeEventListener( mCbId );
		mSubMenu->removeEventListener( mCbId2 );
		mSubMenu->setOwnerNode( NULL );
	}

	mSubMenu = SubMenu;

	if ( NULL != mSubMenu ) {
		mCbId = mSubMenu->addEventListener( Event::OnEnabledChange,
											cb::Make1( this, &UIMenuSubMenu::onSubMenuFocusLoss ) );
		mCbId2 = mSubMenu->addEventListener( Event::OnHideByClick,
											 cb::Make1( this, &UIMenuSubMenu::onHideByClick ) );
		mSubMenu->setOwnerNode( this );
	}
}

UIMenu* UIMenuSubMenu::getSubMenu() const {
	return mSubMenu;
}

void UIMenuSubMenu::showSubMenu() {
	mSubMenu->setParent( getParent()->getParent() );

	Vector2f Pos = getPixelsPosition();
	nodeToWorldTranslation( Pos );
	Pos.x += mSize.getWidth() + getParent()->asType<UIMenu>()->getPadding().Right;

	UIMenu::fixMenuPos( Pos, mSubMenu, getParent()->asType<UIMenu>(), this );

	mSubMenu->getParent()->worldToNode( Pos );
	mSubMenu->setPosition( Pos );

	if ( !mSubMenu->isVisible() ) {
		mSubMenu->show();
	}
}

Uint32 UIMenuSubMenu::onMouseOver( const Vector2i& position, const Uint32& flags ) {
	Action* openMenu = Actions::Runnable::New(
		[&] {
			if ( isMouseOver() )
				showSubMenu();
			mCurWait = NULL;
		},
		mMaxTime );

	runAction( openMenu );
	return UIMenuItem::onMouseOver( position, flags );
}

Uint32 UIMenuSubMenu::onMouseLeave( const Vector2i& Pos, const Uint32& Flags ) {
	UIMenuItem::onMouseLeave( Pos, Flags );
	if ( NULL != mCurWait ) {
		removeAction( mCurWait );
		mCurWait = NULL;
	}
	return UIMenuItem::onMouseLeave( Pos, Flags );
}

UINode* UIMenuSubMenu::getArrow() const {
	return mArrow;
}

void UIMenuSubMenu::onSubMenuFocusLoss( const Event* ) {
	Node* focusCtrl = NULL;

	if ( NULL != getEventDispatcher() ) {
		focusCtrl = getEventDispatcher()->getFocusControl();

		if ( getParent() != focusCtrl && !getParent()->isParentOf( focusCtrl ) ) {
			getParent()->setFocus();
		}
	}

	if ( mSubMenu->mClickHide ) {
		UIMenu* parentMenu = getParent()->asType<UIMenu>();

		if ( !parentMenu->isSubMenu( focusCtrl ) && focusCtrl != this ) {
			parentMenu->sendCommonEvent( Event::OnHideByClick );
			parentMenu->hide();
		}

		mSubMenu->mClickHide = false;
	}
}

void UIMenuSubMenu::onHideByClick( const Event* ) {
	UIMenu* tMenu = getParent()->asType<UIMenu>();

	tMenu->mClickHide = true;
	tMenu->sendCommonEvent( Event::OnHideByClick );
	tMenu->hide();
}

bool UIMenuSubMenu::inheritsFrom( const Uint32 Type ) {
	if ( Type == UI_TYPE_MENUITEM )
		return true;

	return false;
}

const Time& UIMenuSubMenu::getMouseOverTimeShowMenu() const {
	return mMaxTime;
}

void UIMenuSubMenu::setMouseOverTimeShowMenu( const Time& maxTime ) {
	mMaxTime = maxTime;
}

}} // namespace EE::UI
