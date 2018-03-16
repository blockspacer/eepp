#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uieventdispatcher.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/window/window.hpp>
#include <eepp/system/packmanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <pugixml/pugixml.hpp>
#include <eepp/ui/uiwidgetcreator.hpp>
#include <algorithm>

namespace EE { namespace UI {

UISceneNode * UISceneNode::New( EE::Window::Window * window ) {
	return eeNew( UISceneNode, ( window ) );
}

UISceneNode::UISceneNode( EE::Window::Window * window ) :
	SceneNode( window )
{
	mNodeFlags |= NODE_FLAG_UISCENENODE | NODE_FLAG_OVER_FIND_ALLOWED;

	setEventDispatcher( UIEventDispatcher::New( this ) );

	resizeControl( mWindow );
}

void UISceneNode::resizeControl( EE::Window::Window * win ) {
	setSize( (Float)mWindow->getWidth() / PixelDensity::getPixelDensity(), (Float)mWindow->getHeight() / PixelDensity::getPixelDensity() );
	sendMsg( this, NodeMessage::WindowResize );
}

void UISceneNode::setTranslator( Translator translator ) {
	mTranslator = translator;
}

String UISceneNode::getTranslatorString(const std::string & str) {
	if ( String::startsWith( str, "@string/" ) ) {
		String tstr = mTranslator.getString( str.substr( 8 ) );

		if ( !tstr.empty() )
			return tstr;
	}

	return String( str );
}

void UISceneNode::setFocusLastWindow( UIWindow * window ) {
	if ( NULL != mEventDispatcher && !mWindowsList.empty() && window != mWindowsList.front() ) {
		mEventDispatcher->setFocusControl( mWindowsList.front() );
	}
}

void UISceneNode::windowAdd( UIWindow * win ) {
	if ( !windowExists( win ) ) {
		mWindowsList.push_front( win );
	} else {
		//! Send to front
		mWindowsList.remove( win );
		mWindowsList.push_front( win );
	}
}

void UISceneNode::windowRemove( UIWindow * win ) {
	if ( windowExists( win ) ) {
		mWindowsList.remove( win );
	}
}

bool UISceneNode::windowExists( UIWindow * win ) {
	return mWindowsList.end() != std::find( mWindowsList.begin(), mWindowsList.end(), win );
}

UIWidget * UISceneNode::loadLayoutNodes( pugi::xml_node node, Node * parent ) {
	UIWidget * firstWidget = NULL;

	if ( NULL == parent )
		parent = this;

	for ( pugi::xml_node widget = node; widget; widget = widget.next_sibling() ) {
		UIWidget * uiwidget = UIWidgetCreator::createFromName( widget.name() );

		if ( NULL != uiwidget ) {
			if ( NULL == firstWidget ) {
				firstWidget = uiwidget;
			}

			uiwidget->setParent( parent );
			uiwidget->loadFromXmlNode( widget );

			if ( widget.first_child() ) {
				loadLayoutNodes( widget.first_child(), uiwidget );
			}

			uiwidget->onWidgetCreated();
		}
	}

	return firstWidget;
}

UIWidget * UISceneNode::loadLayoutFromFile( const std::string& layoutPath, Node * parent ) {
	if ( FileSystem::fileExists( layoutPath ) ) {
		pugi::xml_document doc;
		pugi::xml_parse_result result = doc.load_file( layoutPath.c_str() );

		if ( result ) {
			return loadLayoutNodes( doc.first_child(), NULL != parent ? parent : this );
		} else {
			eePRINTL( "Error: Couldn't load UI Layout: %s", layoutPath.c_str() );
			eePRINTL( "Error description: %s", result.description() );
			eePRINTL( "Error offset: %d", result.offset );
		}
	} else if ( PackManager::instance()->isFallbackToPacksActive() ) {
		std::string path( layoutPath );
		Pack * pack = PackManager::instance()->exists( path );

		if ( NULL != pack ) {
			return loadLayoutFromPack( pack, path, parent );
		}
	}

	return NULL;
}

UIWidget * UISceneNode::loadLayoutFromString( const std::string& layoutString, Node * parent ) {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_string( layoutString.c_str() );

	if ( result ) {
		return loadLayoutNodes( doc.first_child(), NULL != parent ? parent : this );
	} else {
		eePRINTL( "Error: Couldn't load UI Layout from string: %s", layoutString.c_str() );
		eePRINTL( "Error description: %s", result.description() );
		eePRINTL( "Error offset: %d", result.offset );
	}

	return NULL;
}

UIWidget * UISceneNode::loadLayoutFromMemory( const void * buffer, Int32 bufferSize, Node * parent ) {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_buffer( buffer, bufferSize);

	if ( result ) {
		return loadLayoutNodes( doc.first_child(), NULL != parent ? parent : this );
	} else {
		eePRINTL( "Error: Couldn't load UI Layout from buffer" );
		eePRINTL( "Error description: %s", result.description() );
		eePRINTL( "Error offset: %d", result.offset );
	}

	return NULL;
}

UIWidget * UISceneNode::loadLayoutFromStream( IOStream& stream, Node * parent ) {
	if ( !stream.isOpen() )
		return NULL;

	ios_size bufferSize = stream.getSize();
	SafeDataPointer safeDataPointer( eeNewArray( Uint8, bufferSize ), bufferSize );
	stream.read( reinterpret_cast<char*>( safeDataPointer.data ), safeDataPointer.size );

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_buffer( safeDataPointer.data, safeDataPointer.size );

	if ( result ) {
		return loadLayoutNodes( doc.first_child(), NULL != parent ? parent : this );
	} else {
		eePRINTL( "Error: Couldn't load UI Layout from stream" );
		eePRINTL( "Error description: %s", result.description() );
		eePRINTL( "Error offset: %d", result.offset );
	}

	return NULL;
}

UIWidget * UISceneNode::loadLayoutFromPack( Pack * pack, const std::string& FilePackPath, Node * parent ) {
	SafeDataPointer PData;

	if ( pack->isOpen() && pack->extractFileToMemory( FilePackPath, PData ) ) {
		return loadLayoutFromMemory( PData.data, PData.size, parent );
	}

	return NULL;
}

void UISceneNode::setInternalSize( const Sizef& size ) {
	mDpSize = size;
	mSize = PixelDensity::dpToPx( size );
	updateCenter();
	sendCommonEvent( Event::OnSizeChange );
	invalidateDraw();
}

Node * UISceneNode::setSize( const Sizef & Size ) {
	if ( Size != mDpSize ) {
		Vector2f sizeChange( Size.x - mDpSize.x, Size.y - mDpSize.y );

		setInternalSize( Size );

		onSizeChange();

		if ( reportSizeChangeToChilds() ) {
			sendParentSizeChange( sizeChange );
		}
	}

	return this;
}

Node * UISceneNode::setSize(const Float & Width, const Float & Height) {
	return setSize( Vector2f( Width, Height ) );
}

const Sizef &UISceneNode::getSize() {
	return mDpSize;
}

const Sizef &UISceneNode::getRealSize() {
	return mSize;
}

}}