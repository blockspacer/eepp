#include "codeeditor.hpp"
#include "autocompletemodule.hpp"
#include "lintermodule.hpp"
#include <algorithm>
#include <args/args.hxx>

App* appInstance = nullptr;

static bool isRelativePath( const std::string& path ) {
	if ( !path.empty() ) {
		if ( path[0] == '/' )
			return false;
#if EE_PLATFORM == EE_PLATFORM_WIN
		if ( path.size() >= 2 && String::isLetter( path[0] ) && path[1] == ':' )
			return false;
#endif
	}
	return true;
}

void appLoop() {
	appInstance->mainLoop();
}

bool App::onCloseRequestCallback( EE::Window::Window* ) {
	if ( nullptr != mEditorSplitter->getCurEditor() &&
		 mEditorSplitter->getCurEditor()->isDirty() ) {
		UIMessageBox* msgBox = UIMessageBox::New(
			UIMessageBox::OK_CANCEL,
			"Do you really want to close the code editor?\nAll changes will be lost." );
		msgBox->addEventListener( Event::MsgBoxConfirmClick, [&]( const Event* ) {
			if ( !mCurrentProject.empty() )
				mConfig.saveProject( mCurrentProject, mEditorSplitter, mConfigPath );
			mWindow->close();
		} );
		msgBox->addEventListener( Event::OnClose, [&]( const Event* ) { msgBox = nullptr; } );
		msgBox->setTitle( "Close " + mWindowTitle + "?" );
		msgBox->center();
		msgBox->show();
		return false;
	} else {
		if ( !mCurrentProject.empty() )
			mConfig.saveProject( mCurrentProject, mEditorSplitter, mConfigPath );
		return true;
	}
}

void App::saveDoc() {
	if ( mEditorSplitter->getCurEditor()->getDocument().hasFilepath() ) {
		if ( mEditorSplitter->getCurEditor()->save() )
			updateEditorState();
	} else {
		saveFileDialog( mEditorSplitter->getCurEditor() );
	}
}

void App::saveAllProcess() {
	if ( mTmpDocs.empty() )
		return;

	mEditorSplitter->forEachEditorStoppable( [&]( UICodeEditor* editor ) {
		if ( editor->getDocument().isDirty() &&
			 std::find( mTmpDocs.begin(), mTmpDocs.end(), &editor->getDocument() ) !=
				 mTmpDocs.end() ) {
			if ( editor->getDocument().hasFilepath() ) {
				editor->save();
				updateEditorTabTitle( editor );
				mTmpDocs.erase( &editor->getDocument() );
			} else {
				UIFileDialog* dialog = saveFileDialog( editor, false );
				dialog->addEventListener( Event::SaveFile, [&, editor]( const Event* ) {
					updateEditorTabTitle( editor );
				} );
				dialog->addEventListener( Event::OnWindowClose, [&, editor]( const Event* ) {
					mTmpDocs.erase( &editor->getDocument() );
					if ( !SceneManager::instance()->isShootingDown() && !mTmpDocs.empty() )
						saveAllProcess();
				} );
				return true;
			}
		}
		return false;
	} );
}

void App::saveAll() {
	mEditorSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
		if ( editor->isDirty() )
			mTmpDocs.insert( &editor->getDocument() );
	} );
	saveAllProcess();
}

std::string App::titleFromEditor( UICodeEditor* editor ) {
	std::string title( editor->getDocument().getFilename() );
	return editor->getDocument().isDirty() ? title + "*" : title;
}

void App::updateEditorTabTitle( UICodeEditor* editor ) {
	std::string title( titleFromEditor( editor ) );
	if ( editor->getData() ) {
		UITab* tab = (UITab*)editor->getData();
		tab->setText( title );
	}
}

void App::updateEditorTitle( UICodeEditor* editor ) {
	std::string title( titleFromEditor( editor ) );
	updateEditorTabTitle( editor );
	setAppTitle( title );
}

void App::setAppTitle( const std::string& title ) {
	mWindow->setTitle( mWindowTitle + String( title.empty() ? "" : " - " + title ) );
}

void App::onDocumentModified( UICodeEditor* editor, TextDocument& ) {
	bool isDirty = editor->getDocument().isDirty();
	bool wasDirty =
		!mWindow->getTitle().empty() && mWindow->getTitle()[mWindow->getTitle().size() - 1] == '*';

	if ( isDirty != wasDirty )
		setAppTitle( titleFromEditor( editor ) );

	const String::StringBaseType& tabDirty =
		( (UITab*)editor->getData() )->getText().lastChar() == '*';

	if ( isDirty != tabDirty )
		updateEditorTitle( editor );
}

void App::openFileDialog() {
	UIFileDialog* dialog = UIFileDialog::New( UIFileDialog::DefaultFlags, "*", "." );
	dialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
	dialog->setTitle( "Open File" );
	dialog->setCloseShortcut( KEY_ESCAPE );
	dialog->addEventListener( Event::OpenFile, [&]( const Event* event ) {
		mEditorSplitter->loadFileFromPathInNewTab(
			event->getNode()->asType<UIFileDialog>()->getFullPath() );
	} );
	dialog->addEventListener( Event::OnWindowClose, [&]( const Event* ) {
		if ( mEditorSplitter && mEditorSplitter->getCurEditor() &&
			 !SceneManager::instance()->isShootingDown() )
			mEditorSplitter->getCurEditor()->setFocus();
	} );
	dialog->center();
	dialog->show();
}

void App::openFolderDialog() {
	UIFileDialog* dialog =
		UIFileDialog::New( UIFileDialog::DefaultFlags | UIFileDialog::AllowFolderSelect |
							   UIFileDialog::ShowOnlyFolders,
						   "*", "." );
	dialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
	dialog->setTitle( "Open Folder" );
	dialog->setCloseShortcut( KEY_ESCAPE );
	dialog->addEventListener( Event::OpenFile, [&]( const Event* event ) {
		String path( event->getNode()->asType<UIFileDialog>()->getFullPath() );
		if ( FileSystem::isDirectory( path ) )
			loadFolder( path );
	} );
	dialog->addEventListener( Event::OnWindowClose, [&]( const Event* ) {
		if ( mEditorSplitter && mEditorSplitter->getCurEditor() &&
			 !SceneManager::instance()->isShootingDown() )
			mEditorSplitter->getCurEditor()->setFocus();
	} );
	dialog->center();
	dialog->show();
}

void App::openFontDialog( std::string& fontPath ) {
	std::string absoluteFontPath( fontPath );
	if ( isRelativePath( absoluteFontPath ) )
		absoluteFontPath = mResPath + fontPath;
	UIFileDialog* dialog = UIFileDialog::New( UIFileDialog::DefaultFlags, "*.ttf; *.otf; *.wolff",
											  FileSystem::fileRemoveFileName( absoluteFontPath ) );
	ModelIndex index = dialog->getMultiView()->getListView()->findRowWithText(
		FileSystem::fileNameFromPath( fontPath ), true, true );
	if ( index.isValid() )
		dialog->runOnMainThread(
			[&, dialog, index]() { dialog->getMultiView()->setSelection( index ); } );
	dialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
	dialog->setTitle( "Select Font File" );
	dialog->setCloseShortcut( KEY_ESCAPE );
	dialog->addEventListener( Event::OpenFile, [&]( const Event* event ) {
		auto newPath = event->getNode()->asType<UIFileDialog>()->getFullPath();
		if ( String::startsWith( newPath, mResPath ) )
			newPath = newPath.substr( mResPath.size() );
		if ( fontPath != newPath ) {
			fontPath = newPath;
			auto fontName =
				FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( fontPath ) );
			FontTrueType* fontMono = loadFont( fontName, fontPath );
			if ( fontMono ) {
				mFontMono = fontMono;
				mFontMono->setBoldAdvanceSameAsRegular( true );
				if ( mEditorSplitter ) {
					mEditorSplitter->forEachEditor(
						[&]( UICodeEditor* editor ) { editor->setFont( mFontMono ); } );
				}
			}
		}
	} );
	dialog->addEventListener( Event::OnWindowClose, [&]( const Event* ) {
		if ( mEditorSplitter && mEditorSplitter->getCurEditor() &&
			 !SceneManager::instance()->isShootingDown() )
			mEditorSplitter->getCurEditor()->setFocus();
	} );
	dialog->center();
	dialog->show();
}

UIFileDialog* App::saveFileDialog( UICodeEditor* editor, bool focusOnClose ) {
	if ( !editor )
		return nullptr;
	UIFileDialog* dialog =
		UIFileDialog::New( UIFileDialog::DefaultFlags | UIFileDialog::SaveDialog, "." );
	dialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
	dialog->setTitle( "Save File As" );
	dialog->setCloseShortcut( KEY_ESCAPE );
	std::string filename( editor->getDocument().getFilename() );
	if ( FileSystem::fileExtension( editor->getDocument().getFilename() ).empty() )
		filename += editor->getSyntaxDefinition().getFileExtension();
	dialog->setFileName( filename );
	dialog->addEventListener( Event::SaveFile, [&, editor]( const Event* event ) {
		if ( editor ) {
			std::string path( event->getNode()->asType<UIFileDialog>()->getFullPath() );
			if ( !path.empty() && !FileSystem::isDirectory( path ) &&
				 FileSystem::fileCanWrite( FileSystem::fileRemoveFileName( path ) ) ) {
				if ( editor->getDocument().save( path ) ) {
					updateEditorState();
				} else {
					UIMessageBox* msg =
						UIMessageBox::New( UIMessageBox::OK, "Couldn't write the file." );
					msg->setTitle( "Error" );
					msg->show();
				}
			} else {
				UIMessageBox* msg =
					UIMessageBox::New( UIMessageBox::OK, "You must set a name to the file." );
				msg->setTitle( "Error" );
				msg->show();
			}
		}
	} );
	if ( focusOnClose ) {
		dialog->addEventListener( Event::OnWindowClose, [&, editor]( const Event* ) {
			if ( editor && !SceneManager::instance()->isShootingDown() )
				editor->setFocus();
		} );
	}
	dialog->center();
	dialog->show();
	return dialog;
}

bool App::findPrevText( SearchState& search ) {
	if ( search.text.empty() )
		search.text = mLastSearch;
	if ( !search.editor || !mEditorSplitter->editorExists( search.editor ) || search.text.empty() )
		return false;

	search.editor->getDocument().setActiveClient( search.editor );
	mLastSearch = search.text;
	TextDocument& doc = search.editor->getDocument();
	TextRange range = doc.getDocRange();
	TextPosition from = doc.getSelection( true ).start();
	if ( search.range.isValid() ) {
		range = doc.sanitizeRange( search.range ).normalized();
		from = from < range.start() ? range.start() : from;
	}

	TextPosition found =
		doc.findLast( search.text, from, search.caseSensitive, search.wholeWord, search.range );
	if ( found.isValid() ) {
		doc.setSelection( { doc.positionOffset( found, search.text.size() ), found } );
		return true;
	} else {
		found = doc.findLast( search.text, range.end() );
		if ( found.isValid() ) {
			doc.setSelection( { doc.positionOffset( found, search.text.size() ), found } );
			return true;
		}
	}
	return false;
}

bool App::findNextText( SearchState& search ) {
	if ( search.text.empty() )
		search.text = mLastSearch;
	if ( !search.editor || !mEditorSplitter->editorExists( search.editor ) || search.text.empty() )
		return false;

	search.editor->getDocument().setActiveClient( search.editor );
	mLastSearch = search.text;
	TextDocument& doc = search.editor->getDocument();
	TextRange range = doc.getDocRange();
	TextPosition from = doc.getSelection( true ).end();
	if ( search.range.isValid() ) {
		range = doc.sanitizeRange( search.range ).normalized();
		from = from < range.start() ? range.start() : from;
	}

	TextRange found =
		doc.find( search.text, from, search.caseSensitive, search.wholeWord, search.type, range );
	if ( found.isValid() ) {
		doc.setSelection( found.reversed() );
		return true;
	} else {
		found = doc.find( search.text, range.start(), search.caseSensitive, search.wholeWord,
						  search.type, range );
		if ( found.isValid() ) {
			doc.setSelection( found.reversed() );
			return true;
		}
	}
	return false;
}

bool App::replaceSelection( SearchState& search, const String& replacement ) {
	if ( !search.editor || !mEditorSplitter->editorExists( search.editor ) ||
		 !search.editor->getDocument().hasSelection() )
		return false;
	search.editor->getDocument().setActiveClient( search.editor );
	search.editor->getDocument().replaceSelection( replacement );
	return true;
}

int App::replaceAll( SearchState& search, const String& replace ) {
	if ( !search.editor || !mEditorSplitter->editorExists( search.editor ) )
		return 0;
	if ( search.text.empty() )
		search.text = mLastSearch;
	if ( search.text.empty() )
		return 0;
	search.editor->getDocument().setActiveClient( search.editor );
	mLastSearch = search.text;
	TextDocument& doc = search.editor->getDocument();
	TextPosition startedPosition = doc.getSelection().start();
	int count = doc.replaceAll( search.text, replace, search.caseSensitive, search.wholeWord,
								search.type, search.range );
	doc.setSelection( startedPosition );
	return count;
}

bool App::findAndReplace( SearchState& search, const String& replace ) {
	if ( !search.editor || !mEditorSplitter->editorExists( search.editor ) )
		return false;
	if ( search.text.empty() )
		search.text = mLastSearch;
	if ( search.text.empty() )
		return false;
	search.editor->getDocument().setActiveClient( search.editor );
	mLastSearch = search.text;
	TextDocument& doc = search.editor->getDocument();
	if ( doc.hasSelection() && doc.getSelectedText() == search.text ) {
		return replaceSelection( search, replace );
	} else {
		return findNextText( search );
	}
}

void App::runCommand( const std::string& command ) {
	if ( mEditorSplitter->getCurEditor() )
		mEditorSplitter->getCurEditor()->getDocument().execute( command );
}

void App::loadConfig() {
	mConfig.load( mConfigPath, mKeybindingsPath, mInitColorScheme, mRecentFiles, mRecentFolders,
				  mResPath, mDisplayDPI );
}

void App::saveConfig() {
	mConfig.save( mRecentFiles, mRecentFolders,
				  mProjectSplitter ? mProjectSplitter->getSplitPartition().toString() : "15%",
				  mWindow,
				  mEditorSplitter ? mEditorSplitter->getCurrentColorSchemeName()
								  : mConfig.editor.colorScheme );
}

static std::string keybindFormat( std::string str ) {
	if ( !str.empty() ) {
		str[0] = std::toupper( str[0] );
		size_t found = str.find_first_of( '+' );
		while ( found != std::string::npos ) {
			if ( found + 1 < str.size() ) {
				str[found + 1] = std::toupper( str[found + 1] );
			}
			found = str.find_first_of( '+', found + 1 );
		}
		return str;
	}
	return "";
}

std::string App::getKeybind( const std::string& command ) {
	auto it = mKeybindingsInvert.find( command );
	if ( it != mKeybindingsInvert.end() )
		return keybindFormat( it->second );
	return "";
}

static int LOCATEBAR_MAX_VISIBLE_ITEMS = 18;
static int LOCATEBAR_MAX_RESULTS = 100;

void App::hideLocateBar() {
	mLocateBarLayout->setVisible( false );
	mLocateTable->setVisible( false );
}

void App::updateLocateTable() {
	if ( !mLocateInput->getText().empty() ) {
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
		mDirTree->asyncFuzzyMatchTree(
			mLocateInput->getText(), LOCATEBAR_MAX_RESULTS, [&]( auto res ) {
				mUISceneNode->runOnMainThread( [&, res] {
					mLocateTable->setModel( res );
					mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
				} );
			} );
#else
		mLocateTable->setModel(
			mDirTree->fuzzyMatchTree( mLocateInput->getText(), LOCATEBAR_MAX_RESULTS ) );
		mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
#endif
	} else {
		mLocateTable->setModel( mDirTree->asModel( LOCATEBAR_MAX_RESULTS ) );
		mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
	}
}

bool App::trySendUnlockedCmd( const KeyEvent& keyEvent ) {
	if ( mEditorSplitter->getCurEditor() ) {
		std::string cmd = mEditorSplitter->getCurEditor()->getKeyBindings().getCommandFromKeyBind(
			{ keyEvent.getKeyCode(), keyEvent.getMod() } );
		if ( !cmd.empty() && mEditorSplitter->getCurEditor()->isUnlockedCommand( cmd ) ) {
			mEditorSplitter->getCurEditor()->getDocument().execute( cmd );
			return true;
		}
	}
	return false;
}

void App::goToLine() {
	showLocateBar();
	mLocateInput->setText( "l " );
}

void App::initLocateBar() {
	auto addClickListener = [&]( UIWidget* widget, std::string cmd ) {
		widget->addEventListener( Event::MouseClick, [this, cmd]( const Event* event ) {
			const MouseEvent* mouseEvent = static_cast<const MouseEvent*>( event );
			if ( mouseEvent->getFlags() & EE_BUTTON_LMASK )
				mLocateBarLayout->execute( cmd );
		} );
	};
	mLocateTable = UITableView::New();
	mLocateTable->setId( "locate_bar_table" );
	mLocateTable->setParent( mUISceneNode->getRoot() );
	mLocateTable->setHeadersVisible( false );
	mLocateTable->setVisible( false );
	mLocateInput->addEventListener( Event::OnTextChanged, [&]( const Event* ) {
		if ( mEditorSplitter->getCurEditor() &&
			 String::startsWith( mLocateInput->getText(), String( "l " ) ) ) {
			String number( mLocateInput->getText().substr( 2 ) );
			Int64 val;
			if ( String::fromString( val, number ) && val - 1 >= 0 ) {
				mEditorSplitter->getCurEditor()->goToLine( { val - 1, 0 } );
				mLocateTable->setVisible( false );
			}
		} else {
			mLocateTable->setVisible( true );
			Vector2f pos( mLocateInput->convertToWorldSpace( { 0, 0 } ) );
			pos.y -= mLocateTable->getPixelsSize().getHeight();
			mLocateTable->setPixelsPosition( pos );
			if ( !mDirTreeReady )
				return;
			updateLocateTable();
		}
	} );
	mLocateInput->addEventListener( Event::OnPressEnter, [&]( const Event* ) {
		KeyEvent keyEvent( mLocateTable, Event::KeyDown, KEY_RETURN, 0, 0 );
		mLocateTable->forceKeyDown( keyEvent );
	} );
	mLocateInput->addEventListener( Event::KeyDown, [&]( const Event* event ) {
		const KeyEvent* keyEvent = static_cast<const KeyEvent*>( event );
		mLocateTable->forceKeyDown( *keyEvent );
	} );
	mLocateBarLayout->addCommand( "close-locatebar", [&] {
		hideLocateBar();
		mEditorSplitter->getCurEditor()->setFocus();
	} );
	mLocateBarLayout->getKeyBindings().addKeybindsString( {
		{ "escape", "close-locatebar" },
	} );
	mLocateTable->addEventListener( Event::KeyDown, [&]( const Event* event ) {
		const KeyEvent* keyEvent = static_cast<const KeyEvent*>( event );
		if ( keyEvent->getKeyCode() == KEY_ESCAPE )
			mLocateBarLayout->execute( "close-locatebar" );
	} );
	addClickListener( mLocateBarLayout->find<UIWidget>( "locatebar_close" ), "close-locatebar" );
	mLocateTable->addEventListener( Event::OnModelEvent, [&]( const Event* event ) {
		const ModelEvent* modelEvent = static_cast<const ModelEvent*>( event );
		if ( modelEvent->getModelEventType() == ModelEventType::Open ) {
			Variant vPath( modelEvent->getModel()->data(
				modelEvent->getModel()->index( modelEvent->getModelIndex().row(), 1 ),
				Model::Role::Display ) );
			if ( vPath.isValid() && vPath.is( Variant::Type::cstr ) ) {
				std::string path( vPath.asCStr() );
				UITab* tab = mEditorSplitter->isDocumentOpen( path );
				if ( !tab ) {
					FileInfo fileInfo( path );
					if ( fileInfo.exists() && fileInfo.isRegularFile() )
						mEditorSplitter->loadFileFromPathInNewTab( path );
				} else {
					tab->getTabWidget()->setTabSelected( tab );
				}
				mLocateBarLayout->execute( "close-locatebar" );
			}
		}
	} );
}

void App::updateLocateBar() {
	mLocateBarLayout->runOnMainThread( [&] {
		Float width = eeceil( mLocateInput->getPixelsSize().getWidth() );
		mLocateTable->setPixelsSize( width,
									 mLocateTable->getRowHeight() * LOCATEBAR_MAX_VISIBLE_ITEMS );
		width -= mLocateTable->getVerticalScrollBar()->getPixelsSize().getWidth();
		mLocateTable->setColumnWidth( 0, eeceil( width * 0.5 ) );
		mLocateTable->setColumnWidth( 1, width - mLocateTable->getColumnWidth( 0 ) );
		Vector2f pos( mLocateInput->convertToWorldSpace( { 0, 0 } ) );
		pos.y -= mLocateTable->getPixelsSize().getHeight();
		mLocateTable->setPixelsPosition( pos );
	} );
}

void App::hideSearchBar() {
	mSearchBarLayout->setEnabled( false )->setVisible( false );
}

void App::showLocateBar() {
	hideGlobalSearchBar();
	hideSearchBar();

	mLocateBarLayout->setVisible( true );
	mLocateInput->setFocus();
	mLocateTable->setVisible( true );
	mLocateInput->getDocument().selectAll();
	mLocateInput->addEventListener( Event::OnSizeChange,
									[&]( const Event* ) { updateLocateBar(); } );
	if ( mDirTree && !mLocateTable->getModel() ) {
		mLocateTable->setModel( mDirTree->asModel( LOCATEBAR_MAX_RESULTS ) );
		mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
	}
	updateLocateBar();
}

void App::initSearchBar() {
	auto addClickListener = [&]( UIWidget* widget, std::string cmd ) {
		widget->addEventListener( Event::MouseClick, [this, cmd]( const Event* event ) {
			const MouseEvent* mouseEvent = static_cast<const MouseEvent*>( event );
			if ( mouseEvent->getFlags() & EE_BUTTON_LMASK )
				mSearchBarLayout->execute( cmd );
		} );
	};
	auto addReturnListener = [&]( UIWidget* widget, std::string cmd ) {
		widget->addEventListener( Event::OnPressEnter, [this, cmd]( const Event* ) {
			mSearchBarLayout->execute( cmd );
		} );
	};
	UITextInput* findInput = mSearchBarLayout->find<UITextInput>( "search_find" );
	UITextInput* replaceInput = mSearchBarLayout->find<UITextInput>( "search_replace" );
	UICheckBox* caseSensitiveChk = mSearchBarLayout->find<UICheckBox>( "case_sensitive" );
	UICheckBox* wholeWordChk = mSearchBarLayout->find<UICheckBox>( "whole_word" );
	UICheckBox* luaPatternChk = mSearchBarLayout->find<UICheckBox>( "lua_pattern" );

	caseSensitiveChk->addEventListener(
		Event::OnValueChange, [&, caseSensitiveChk]( const Event* ) {
			mSearchState.caseSensitive = caseSensitiveChk->isChecked();
		} );

	wholeWordChk->addEventListener( Event::OnValueChange, [&, wholeWordChk]( const Event* ) {
		mSearchState.wholeWord = wholeWordChk->isChecked();
	} );

	luaPatternChk->addEventListener( Event::OnValueChange, [&, luaPatternChk]( const Event* ) {
		mSearchState.type = luaPatternChk->isChecked() ? TextDocument::FindReplaceType::LuaPattern
													   : TextDocument::FindReplaceType::Normal;
	} );

	findInput->addEventListener( Event::OnTextChanged, [&, findInput]( const Event* ) {
		if ( mSearchState.editor && mEditorSplitter->editorExists( mSearchState.editor ) ) {
			mSearchState.text = findInput->getText();
			mSearchState.editor->setHighlightWord( mSearchState.text );
			if ( !mSearchState.text.empty() ) {
				mSearchState.editor->getDocument().setSelection( { 0, 0 } );
				if ( !findNextText( mSearchState ) ) {
					findInput->addClass( "error" );
				} else {
					findInput->removeClass( "error" );
				}
			} else {
				findInput->removeClass( "error" );
				mSearchState.editor->getDocument().setSelection(
					mSearchState.editor->getDocument().getSelection().start() );
			}
		}
	} );
	mSearchBarLayout->addCommand( "close-searchbar", [&] {
		hideSearchBar();
		if ( mEditorSplitter->getCurEditor() )
			mEditorSplitter->getCurEditor()->setFocus();
		if ( mSearchState.editor ) {
			if ( mEditorSplitter->editorExists( mSearchState.editor ) ) {
				mSearchState.editor->setHighlightWord( "" );
				mSearchState.editor->setHighlightTextRange( TextRange() );
			}
		}
	} );
	mSearchBarLayout->addCommand( "repeat-find", [this] { findNextText( mSearchState ); } );
	mSearchBarLayout->addCommand( "replace-all", [this, replaceInput] {
		replaceAll( mSearchState, replaceInput->getText() );
		replaceInput->setFocus();
	} );
	mSearchBarLayout->addCommand( "find-and-replace", [this, replaceInput] {
		findAndReplace( mSearchState, replaceInput->getText() );
	} );
	mSearchBarLayout->addCommand( "find-prev", [this] { findPrevText( mSearchState ); } );
	mSearchBarLayout->addCommand( "replace-selection", [this, replaceInput] {
		replaceSelection( mSearchState, replaceInput->getText() );
	} );
	mSearchBarLayout->addCommand( "change-case", [&, caseSensitiveChk] {
		caseSensitiveChk->setChecked( !caseSensitiveChk->isChecked() );
	} );
	mSearchBarLayout->addCommand( "change-whole-word", [&, wholeWordChk] {
		wholeWordChk->setChecked( !wholeWordChk->isChecked() );
	} );
	mSearchBarLayout->addCommand( "toggle-lua-pattern", [&, luaPatternChk] {
		luaPatternChk->setChecked( !luaPatternChk->isChecked() );
	} );
	mSearchBarLayout->getKeyBindings().addKeybindsString( { { "f3", "repeat-find" },
															{ "ctrl+g", "repeat-find" },
															{ "escape", "close-searchbar" },
															{ "ctrl+r", "replace-all" },
															{ "ctrl+s", "change-case" },
															{ "ctrl+w", "change-whole-word" },
															{ "ctrl+l", "toggle-lua-pattern" } } );
	addReturnListener( findInput, "repeat-find" );
	addReturnListener( replaceInput, "find-and-replace" );
	addClickListener( mSearchBarLayout->find<UIPushButton>( "find_prev" ), "find-prev" );
	addClickListener( mSearchBarLayout->find<UIPushButton>( "find_next" ), "repeat-find" );
	addClickListener( mSearchBarLayout->find<UIPushButton>( "replace" ), "replace-selection" );
	addClickListener( mSearchBarLayout->find<UIPushButton>( "replace_find" ), "find-and-replace" );
	addClickListener( mSearchBarLayout->find<UIPushButton>( "replace_all" ), "replace-all" );
	addClickListener( mSearchBarLayout->find<UIWidget>( "searchbar_close" ), "close-searchbar" );
	replaceInput->addEventListener( Event::OnTabNavigate,
									[findInput]( const Event* ) { findInput->setFocus(); } );
}

void App::showGlobalSearch() {
	hideLocateBar();
	hideSearchBar();
	mGlobalSearchBarLayout->setVisible( true )->setEnabled( true );
	mGlobalSearchInput->setFocus();
	mGlobalSearchTree->setVisible( true );
	if ( mEditorSplitter->getCurEditor() &&
		 mEditorSplitter->getCurEditor()->getDocument().hasSelection() ) {
		mGlobalSearchInput->setText(
			mEditorSplitter->getCurEditor()->getDocument().getSelectedText() );
	}
	mGlobalSearchInput->getDocument().selectAll();
	auto* loader = mGlobalSearchTree->getParent()->find( "loader" );
	if ( loader )
		loader->setVisible( true );
	updateGlobalSearchBar();
}

void App::updateGlobalSearchBar() {
	mGlobalSearchBarLayout->runOnMainThread( [&] {
		Float width = eeceil( mGlobalSearchInput->getPixelsSize().getWidth() );
		Float rowHeight = mGlobalSearchTree->getRowHeight() * LOCATEBAR_MAX_VISIBLE_ITEMS;
		mGlobalSearchTree->setPixelsSize( width, rowHeight );
		width -= mGlobalSearchTree->getVerticalScrollBar()->getPixelsSize().getWidth();
		mGlobalSearchTree->setColumnWidth( 0, eeceil( width ) );
		Vector2f pos( mGlobalSearchInput->convertToWorldSpace( { 0, 0 } ) );
		pos.y -= mGlobalSearchTree->getPixelsSize().getHeight();
		mGlobalSearchTree->setPixelsPosition( pos );
	} );
}

void App::hideGlobalSearchBar() {
	mGlobalSearchBarLayout->setEnabled( false )->setVisible( false );
	mGlobalSearchTree->setVisible( false );
	auto* loader = mGlobalSearchTree->getParent()->find( "loader" );
	if ( loader )
		loader->setVisible( false );
}

void App::updateGlobalSearchBarResults( const std::string& search,
										std::shared_ptr<ProjectSearch::ResultModel> model ) {
	updateGlobalSearchBar();
	mGlobalSearchTree->setSearchStr( search );
	mGlobalSearchTree->setModel( model );
	if ( mGlobalSearchTree->getModel()->rowCount() < 50 )
		mGlobalSearchTree->expandAll();
}

void App::initGlobalSearchBar() {
	auto addClickListener = [&]( UIWidget* widget, std::string cmd ) {
		widget->addEventListener( Event::MouseClick, [this, cmd]( const Event* event ) {
			const MouseEvent* mouseEvent = static_cast<const MouseEvent*>( event );
			if ( mouseEvent->getFlags() & EE_BUTTON_LMASK )
				mGlobalSearchBarLayout->execute( cmd );
		} );
	};
	UIPushButton* searchButton = mGlobalSearchBarLayout->find<UIPushButton>( "global_search" );
	UICheckBox* caseSensitiveChk = mGlobalSearchBarLayout->find<UICheckBox>( "case_sensitive" );
	UICheckBox* wholeWordChk = mGlobalSearchBarLayout->find<UICheckBox>( "whole_word" );
	UIWidget* searchBarClose = mGlobalSearchBarLayout->find<UIWidget>( "global_searchbar_close" );
	mGlobalSearchInput = mGlobalSearchBarLayout->find<UITextInput>( "global_search_find" );
	mGlobalSearchHistoryList =
		mGlobalSearchBarLayout->find<UIDropDownList>( "global_search_history" );
	mGlobalSearchBarLayout->addCommand( "search-in-files", [&, caseSensitiveChk, wholeWordChk] {
		if ( mDirTree && mDirTree->getFilesCount() > 0 && !mGlobalSearchInput->getText().empty() ) {
			UILoader* loader = UILoader::New();
			loader->setId( "loader" );
			loader->setRadius( 48 );
			loader->setOutlineThickness( 6 );
			loader->setFillColor( Color::Red );
			loader->setParent( mGlobalSearchTree->getParent() );
			loader->setPosition( mGlobalSearchTree->getPosition() +
								 mGlobalSearchTree->getSize() * 0.5f - loader->getSize() * 0.5f );
			Clock* clock = eeNew( Clock, () );
			std::string search( mGlobalSearchInput->getText().toUtf8() );
			ProjectSearch::find(
				mDirTree->getFiles(), search,
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
				mThreadPool,
#endif
				[&, clock, search, loader]( const ProjectSearch::Result& res ) {
					Log::info( "Global search for \"%s\" took %.2fms", search.c_str(),
							   clock->getElapsedTime().asMilliseconds() );
					eeDelete( clock );
					mUISceneNode->runOnMainThread( [&, loader, res, search] {
						auto model = ProjectSearch::asModel( res );
						mGlobalSearchHistory.push_back( std::make_pair( search, model ) );
						if ( mGlobalSearchHistory.size() > 10 )
							mGlobalSearchHistory.pop_front();

						std::vector<String> items;
						for ( auto item = mGlobalSearchHistory.rbegin();
							  item != mGlobalSearchHistory.rend(); item++ ) {
							items.push_back( item->first );
						}

						auto listBox = mGlobalSearchHistoryList->getListBox();
						listBox->clear();
						listBox->addListBoxItems( items );
						if ( mGlobalSearchHistoryOnItemSelectedCb )
							mGlobalSearchHistoryList->removeEventListener(
								mGlobalSearchHistoryOnItemSelectedCb );
						listBox->setSelected( 0 );
						mGlobalSearchHistoryOnItemSelectedCb =
							mGlobalSearchHistoryList->addEventListener(
								Event::OnItemSelected, [&]( const Event* ) {
									auto idx = mGlobalSearchHistoryList->getListBox()
												   ->getItemSelectedIndex();
									auto idxItem = mGlobalSearchHistory.at(
										mGlobalSearchHistory.size() - 1 - idx );
									updateGlobalSearchBarResults( idxItem.first, idxItem.second );
								} );

						updateGlobalSearchBarResults( search, model );
						loader->setVisible( false );
						loader->close();
					} );
				},
				caseSensitiveChk->isChecked(), wholeWordChk->isChecked() );
		}
	} );
	mGlobalSearchBarLayout->addCommand( "close-global-searchbar", [&] {
		hideGlobalSearchBar();
		if ( mEditorSplitter->getCurEditor() )
			mEditorSplitter->getCurEditor()->setFocus();
	} );
	mGlobalSearchBarLayout->getKeyBindings().addKeybindsString( {
		{ "escape", "close-global-searchbar" },
		{ "ctrl+s", "change-case" },
		{ "ctrl+w", "change-whole-word" },
	} );
	mGlobalSearchBarLayout->addCommand( "change-case", [&, caseSensitiveChk] {
		caseSensitiveChk->setChecked( !caseSensitiveChk->isChecked() );
	} );
	mGlobalSearchBarLayout->addCommand( "change-whole-word", [&, wholeWordChk] {
		wholeWordChk->setChecked( !wholeWordChk->isChecked() );
	} );
	mGlobalSearchInput->addEventListener( Event::OnPressEnter, [&]( const Event* ) {
		if ( mGlobalSearchInput->hasFocus() ) {
			mGlobalSearchBarLayout->execute( "search-in-files" );
		} else {
			KeyEvent keyEvent( mGlobalSearchTree, Event::KeyDown, KEY_RETURN, 0, 0 );
			mGlobalSearchTree->forceKeyDown( keyEvent );
		}
	} );
	mGlobalSearchInput->addEventListener( Event::KeyDown, [&]( const Event* event ) {
		const KeyEvent* keyEvent = static_cast<const KeyEvent*>( event );
		Uint32 keyCode = keyEvent->getKeyCode();
		if ( ( keyCode == KEY_UP || keyCode == KEY_DOWN || keyCode == KEY_PAGEUP ||
			   keyCode == KEY_PAGEDOWN || keyCode == KEY_HOME || keyCode == KEY_END ) &&
			 mGlobalSearchTree->forceKeyDown( *keyEvent ) && !mGlobalSearchTree->hasFocus() ) {
			mGlobalSearchTree->setFocus();
		}
	} );
	addClickListener( searchButton, "search-in-files" );
	addClickListener( searchBarClose, "close-global-searchbar" );
	mGlobalSearchTree = UITreeViewGlobalSearch::New( mEditorSplitter->getCurrentColorScheme() );
	mGlobalSearchTree->setId( "search_tree" );
	mGlobalSearchTree->setParent( mUISceneNode->getRoot() );
	mGlobalSearchTree->setExpanderIconSize( PixelDensity::dpToPx( 20 ) );
	mGlobalSearchTree->setHeadersVisible( false );
	mGlobalSearchTree->setVisible( false );
	mGlobalSearchTree->setColumnsHidden(
		{ ProjectSearch::ResultModel::Line, ProjectSearch::ResultModel::ColumnPosition }, true );
	mGlobalSearchTree->addEventListener( Event::KeyDown, [&]( const Event* event ) {
		const KeyEvent* keyEvent = static_cast<const KeyEvent*>( event );
		if ( keyEvent->getKeyCode() == KEY_ESCAPE )
			mGlobalSearchBarLayout->execute( "close-global-searchbar" );
	} );
	mGlobalSearchTree->addEventListener( Event::OnModelEvent, [&]( const Event* event ) {
		const ModelEvent* modelEvent = static_cast<const ModelEvent*>( event );
		if ( modelEvent->getModelEventType() == ModelEventType::Open ) {
			const Model* model = modelEvent->getModel();
			if ( !model )
				return;
			Variant vPath( model->data( model->index( modelEvent->getModelIndex().internalId(),
													  ProjectSearch::ResultModel::FileOrPosition ),
										Model::Role::Custom ) );
			if ( vPath.isValid() && vPath.is( Variant::Type::cstr ) ) {
				std::string path( vPath.asCStr() );
				UITab* tab = mEditorSplitter->isDocumentOpen( path );
				if ( !tab ) {
					FileInfo fileInfo( path );
					if ( fileInfo.exists() && fileInfo.isRegularFile() )
						mEditorSplitter->loadFileFromPathInNewTab( path );
				} else {
					tab->getTabWidget()->setTabSelected( tab );
				}
				Variant lineNum(
					model->data( model->index( modelEvent->getModelIndex().row(),
											   ProjectSearch::ResultModel::FileOrPosition,
											   modelEvent->getModelIndex().parent() ),
								 Model::Role::Custom ) );
				Variant colNum(
					model->data( model->index( modelEvent->getModelIndex().row(),
											   ProjectSearch::ResultModel::ColumnPosition,
											   modelEvent->getModelIndex().parent() ),
								 Model::Role::Custom ) );
				if ( mEditorSplitter->getCurEditor() && lineNum.isValid() && colNum.isValid() &&
					 lineNum.is( Variant::Type::Int64 ) && colNum.is( Variant::Type::Int64 ) ) {
					TextPosition pos{ lineNum.asInt64(), colNum.asInt64() };
					mEditorSplitter->getCurEditor()->getDocument().setSelection( pos );
					mEditorSplitter->getCurEditor()->goToLine( pos );
					hideGlobalSearchBar();
				}
			}
		}
	} );
}

void App::showFindView() {
	hideLocateBar();
	hideGlobalSearchBar();

	UICodeEditor* editor = mEditorSplitter->getCurEditor();
	if ( !editor )
		return;

	mSearchState.editor = editor;
	mSearchState.range = TextRange();
	mSearchState.caseSensitive =
		mSearchBarLayout->find<UICheckBox>( "case_sensitive" )->isChecked();
	mSearchState.wholeWord = mSearchBarLayout->find<UICheckBox>( "whole_word" )->isChecked();
	mSearchBarLayout->setEnabled( true )->setVisible( true );

	UITextInput* findInput = mSearchBarLayout->find<UITextInput>( "search_find" );
	findInput->getDocument().selectAll();
	findInput->setFocus();

	const TextDocument& doc = editor->getDocument();

	if ( doc.getSelection().hasSelection() && doc.getSelection().inSameLine() ) {
		String text = doc.getSelectedText();
		if ( !text.empty() ) {
			findInput->setText( text );
			findInput->getDocument().selectAll();
		} else if ( !findInput->getText().empty() ) {
			findInput->getDocument().selectAll();
		}
	} else if ( doc.getSelection().hasSelection() ) {
		mSearchState.range = doc.getSelection( true );
		if ( !findInput->getText().empty() )
			findInput->getDocument().selectAll();
	}
	mSearchState.text = findInput->getText();
	editor->setHighlightTextRange( mSearchState.range );
	editor->setHighlightWord( mSearchState.text );
	editor->getDocument().setActiveClient( editor );
}

void App::closeApp() {
	if ( onCloseRequestCallback( mWindow ) )
		mWindow->close();
}

void App::mainLoop() {
	mWindow->getInput()->update();
	Time elapsed = SceneManager::instance()->getElapsed();
	SceneManager::instance()->update();

	if ( SceneManager::instance()->getUISceneNode()->invalidated() || mConsole->isActive() ||
		 mConsole->isFading() ) {
		mWindow->clear();
		SceneManager::instance()->draw();
		mConsole->draw( elapsed );
		mWindow->display();
	} else {
		mWindow->getInput()->waitEvent( Milliseconds( mWindow->hasFocus() ? 16 : 100 ) );
	}
}

void App::onFileDropped( String file ) {
	Vector2f mousePos( mUISceneNode->getEventDispatcher()->getMousePosf() );
	Node* node = mUISceneNode->overFind( mousePos );
	UICodeEditor* codeEditor = mEditorSplitter->getCurEditor();
	if ( !node )
		node = codeEditor;
	if ( node && node->isType( UI_TYPE_CODEEDITOR ) ) {
		codeEditor = node->asType<UICodeEditor>();
		if ( !codeEditor->getDocument().isEmpty() ) {
			auto d = mEditorSplitter->createCodeEditorInTabWidget(
				mEditorSplitter->tabWidgetFromEditor( codeEditor ) );
			codeEditor = d.second;
			d.first->getTabWidget()->setTabSelected( d.first );
		}
	}
	mEditorSplitter->loadFileFromPath( file, codeEditor );
}

void App::onTextDropped( String text ) {
	Vector2f mousePos( mUISceneNode->getEventDispatcher()->getMousePosf() );
	Node* node = mUISceneNode->overFind( mousePos );
	UICodeEditor* codeEditor = mEditorSplitter->getCurEditor();
	if ( node && node->isType( UI_TYPE_CODEEDITOR ) )
		codeEditor = node->asType<UICodeEditor>();
	if ( codeEditor && !text.empty() ) {
		if ( text[text.size() - 1] != '\n' )
			text += '\n';
		codeEditor->getDocument().textInput( text );
	}
}

App::App() : mThreadPool( ThreadPool::createShared( eemax<int>( 2, Sys::getCPUCount() ) ) ) {}

App::~App() {
	saveConfig();
	eeSAFE_DELETE( mEditorSplitter );
	eeSAFE_DELETE( mAutoCompleteModule );
	eeSAFE_DELETE( mLinterModule );
	eeSAFE_DELETE( mConsole );
	if ( mFileWatcher )
		delete mFileWatcher;
	if ( mFileSystemListener )
		delete mFileSystemListener;
}

void App::updateRecentFiles() {
	UINode* node = nullptr;
	if ( mSettingsMenu && ( node = mSettingsMenu->getItem( "Recent Files" ) ) ) {
		UIMenuSubMenu* uiMenuSubMenu = static_cast<UIMenuSubMenu*>( node );
		UIMenu* menu = uiMenuSubMenu->getSubMenu();
		uiMenuSubMenu->setEnabled( !mRecentFiles.empty() );
		menu->removeAll();
		menu->removeEventsOfType( Event::OnItemClicked );
		if ( mRecentFiles.empty() )
			return;
		for ( auto file : mRecentFiles )
			menu->add( file );
		menu->addSeparator();
		menu->add( "Clear Menu" );
		menu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
			if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
				return;
			const String& txt = event->getNode()->asType<UIMenuItem>()->getText();
			if ( txt != "Clear Menu" ) {
				std::string path( txt.toUtf8() );
				if ( FileSystem::fileExists( path ) && !FileSystem::isDirectory( path ) ) {
					mEditorSplitter->loadFileFromPathInNewTab( path );
				}
			} else {
				mRecentFiles.clear();
				updateRecentFiles();
			}
		} );
	}
}

void App::updateRecentFolders() {
	UINode* node = nullptr;
	if ( mSettingsMenu && ( node = mSettingsMenu->getItem( "Recent Folders" ) ) ) {
		UIMenuSubMenu* uiMenuSubMenu = static_cast<UIMenuSubMenu*>( node );
		UIMenu* menu = uiMenuSubMenu->getSubMenu();
		uiMenuSubMenu->setEnabled( !mRecentFolders.empty() );
		menu->removeAll();
		menu->removeEventsOfType( Event::OnItemClicked );
		if ( mRecentFolders.empty() )
			return;
		for ( auto file : mRecentFolders )
			menu->add( file );
		menu->addSeparator();
		menu->add( "Clear Menu" );
		menu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
			if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
				return;
			const String& txt = event->getNode()->asType<UIMenuItem>()->getText();
			if ( txt != "Clear Menu" ) {
				loadFolder( txt );
			} else {
				mRecentFolders.clear();
				updateRecentFolders();
			}
		} );
	}
}

void App::showSidePanel( bool show ) {
	if ( show == mSidePanel->isVisible() )
		return;

	if ( show ) {
		mSidePanel->setVisible( true );
		mSidePanel->setParent( mProjectSplitter );
		mProjectSplitter->swap();
	} else {
		mSidePanel->setVisible( false );
		mSidePanel->setParent( mUISceneNode->getRoot() );
	}
}

void App::switchSidePanel() {
	mConfig.ui.showSidePanel = !mConfig.ui.showSidePanel;
	mWindowMenu->getItem( "Show Left Sidebar" )
		->asType<UIMenuCheckBox>()
		->setActive( mConfig.ui.showSidePanel );
	showSidePanel( mConfig.ui.showSidePanel );
}

UIMenu* App::createWindowMenu() {
	mWindowMenu = UIPopUpMenu::New();
	mWindowMenu->add( "UI Scale Factor (Pixel Density)", findIcon( "pixel-density" ) );
	mWindowMenu->add( "UI Font Size", findIcon( "font-size" ) );
	mWindowMenu->add( "Editor Font Size", findIcon( "font-size" ) );
	mWindowMenu->add( "Serif Font...", findIcon( "font-size" ) );
	mWindowMenu->add( "Monospace Font...", findIcon( "font-size" ) );
	mWindowMenu->addSeparator();
	mWindowMenu->addCheckBox( "Full Screen Mode", false, getKeybind( "fullscreen-toggle" ) )
		->setId( "fullscreen-mode" );
	mWindowMenu->addCheckBox( "Show Left Sidebar", mConfig.ui.showSidePanel,
							  getKeybind( "switch-side-panel" ) );
	mWindowMenu->addSeparator();
	mWindowMenu->add( "Split Left", findIcon( "split-horizontal" ), getKeybind( "split-left" ) );
	mWindowMenu->add( "Split Right", findIcon( "split-horizontal" ), getKeybind( "split-right" ) );
	mWindowMenu->add( "Split Top", findIcon( "split-vertical" ), getKeybind( "split-top" ) );
	mWindowMenu->add( "Split Bottom", findIcon( "split-vertical" ), getKeybind( "split-bottom" ) );
	mWindowMenu->addSeparator();
	mWindowMenu->add( "Zoom In", findIcon( "zoom-in" ), getKeybind( "font-size-grow" ) );
	mWindowMenu->add( "Zoom Out", findIcon( "zoom-out" ), getKeybind( "font-size-shrink" ) );
	mWindowMenu->add( "Zoom Reset", findIcon( "zoom-reset" ), getKeybind( "font-size-reset" ) );
	mWindowMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		if ( item->getText() == "Show Left Sidebar" ) {
			mConfig.ui.showSidePanel = item->asType<UIMenuCheckBox>()->isActive();
			showSidePanel( mConfig.ui.showSidePanel );
		} else if ( item->getText() == "UI Scale Factor (Pixel Density)" ) {
			UIMessageBox* msgBox = UIMessageBox::New(
				UIMessageBox::INPUT, "Set the UI scale factor (pixel density):\nMinimum value is "
									 "1, and maximum 6. Requires restart." );
			msgBox->setTitle( mWindowTitle );
			msgBox->getTextInput()->setText(
				String::format( "%.2f", mConfig.window.pixelDensity ) );
			msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
			msgBox->show();
			msgBox->addEventListener( Event::MsgBoxConfirmClick, [&, msgBox]( const Event* ) {
				msgBox->closeWindow();
				Float val;
				if ( String::fromString( val, msgBox->getTextInput()->getText() ) && val >= 1 &&
					 val <= 6 ) {
					if ( mConfig.window.pixelDensity != val ) {
						mConfig.window.pixelDensity = val;
						UIMessageBox* msg = UIMessageBox::New(
							UIMessageBox::OK,
							"New UI scale factor assigned.\nPlease restart the application." );
						msg->show();
						setFocusEditorOnClose( msg );
					} else if ( mEditorSplitter && mEditorSplitter->getCurEditor() ) {
						mEditorSplitter->getCurEditor()->setFocus();
					}
				} else {
					UIMessageBox* msg = UIMessageBox::New( UIMessageBox::OK, "Invalid value!" );
					msg->show();
					setFocusEditorOnClose( msg );
				}
			} );
		} else if ( item->getText() == "Editor Font Size" ) {
			UIMessageBox* msgBox =
				UIMessageBox::New( UIMessageBox::INPUT, "Set the editor font size:" );
			msgBox->setTitle( mWindowTitle );
			msgBox->getTextInput()->setText( mConfig.editor.fontSize.toString() );
			msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
			msgBox->show();
			msgBox->addEventListener( Event::MsgBoxConfirmClick, [&, msgBox]( const Event* ) {
				mConfig.editor.fontSize = StyleSheetLength( msgBox->getTextInput()->getText() );
				mEditorSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
					editor->setFontSize( mConfig.editor.fontSize.asDp( 0, Sizef(), mDisplayDPI ) );
				} );
			} );
			setFocusEditorOnClose( msgBox );
		} else if ( item->getText() == "UI Font Size" ) {
			UIMessageBox* msgBox =
				UIMessageBox::New( UIMessageBox::INPUT, "Set the UI font size:" );
			msgBox->setTitle( mWindowTitle );
			msgBox->getTextInput()->setText( mConfig.ui.fontSize.toString() );
			msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
			msgBox->show();
			msgBox->addEventListener( Event::MsgBoxConfirmClick, [&, msgBox]( const Event* ) {
				mConfig.ui.fontSize = StyleSheetLength( msgBox->getTextInput()->getText() );
				Float fontSize = mConfig.ui.fontSize.asDp( 0, Sizef(), mDisplayDPI );
				UIThemeManager* manager = mUISceneNode->getUIThemeManager();
				manager->setDefaultFontSize( fontSize );
				manager->getDefaultTheme()->setDefaultFontSize( fontSize );
				mUISceneNode->forEachNode( [&]( Node* node ) {
					if ( node->isType( UI_TYPE_TEXTVIEW ) ) {
						UITextView* textView = node->asType<UITextView>();
						if ( !textView->getUIStyle()->hasProperty( PropertyId::FontSize ) ) {
							textView->setFontSize( mConfig.ui.fontSize.asDp(
								node->getParent()->getPixelsSize().getWidth(), Sizef(),
								mUISceneNode->getDPI() ) );
						}
					}
				} );
				msgBox->closeWindow();
			} );
			setFocusEditorOnClose( msgBox );
		} else if ( item->getText() == "Serif Font..." ) {
			openFontDialog( mConfig.ui.serifFont );
		} else if ( item->getText() == "Monospace Font..." ) {
			openFontDialog( mConfig.ui.monospaceFont );
		} else if ( "Zoom In" == item->getText() ) {
			mEditorSplitter->zoomIn();
		} else if ( "Zoom Out" == item->getText() ) {
			mEditorSplitter->zoomOut();
		} else if ( "Zoom Reset" == item->getText() ) {
			mEditorSplitter->zoomReset();
		} else if ( "Full Screen Mode" == item->getText() ) {
			runCommand( "fullscreen-toggle" );
		} else {
			String text = String( event->getNode()->asType<UIMenuItem>()->getText() ).toLower();
			String::replaceAll( text, " ", "-" );
			String::replaceAll( text, "/", "-" );
			runCommand( text );
		}
	} );
	return mWindowMenu;
}

UIMenu* App::createViewMenu() {
	mViewMenu = UIPopUpMenu::New();
	mViewMenu->addCheckBox( "Show Line Numbers" )->setActive( mConfig.editor.showLineNumbers );
	mViewMenu->addCheckBox( "Show White Space" )->setActive( mConfig.editor.showWhiteSpaces );
	mViewMenu->addCheckBox( "Show Document Info" )->setActive( mConfig.editor.showDocInfo );
	mViewMenu->addCheckBox( "Highlight Matching Bracket" )
		->setActive( mConfig.editor.highlightMatchingBracket );
	mViewMenu->addCheckBox( "Highlight Current Line" )
		->setActive( mConfig.editor.highlightCurrentLine );
	mViewMenu->addCheckBox( "Highlight Selection Match" )
		->setActive( mConfig.editor.highlightSelectionMatch );
	mViewMenu->addCheckBox( "Enable Horizontal ScrollBar" )
		->setActive( mConfig.editor.horizontalScrollbar );
	mViewMenu->addCheckBox( "Enable Color Preview" )
		->setActive( mConfig.editor.colorPreview )
		->setTooltipText( "Enables a quick preview of a color when the mouse\n"
						  "is hover a word that represents a color." );
	mViewMenu->addCheckBox( "Enable Color Picker" )
		->setActive( mConfig.editor.colorPickerSelection )
		->setTooltipText( "Enables the color picker tool when a double click selection\n"
						  "is done over a word representing a color." );
	mViewMenu->addCheckBox( "Enable Auto Complete" )
		->setActive( mConfig.editor.autoComplete )
		->setTooltipText( "Auto complete shows the completion popup as you type, so you can fill\n"
						  "in long words by typing only a few characters." );
	mViewMenu->addCheckBox( "Enable Linter" )
		->setActive( mConfig.editor.linter )
		->setTooltipText( "Use static code analysis tool used to flag programming errors, bugs,\n"
						  "stylistic errors, and suspicious constructs." );
	mViewMenu->addCheckBox( "Hide tabbar on single tab" )
		->setActive( mConfig.editor.hideTabBarOnSingleTab )
		->setTooltipText( "Hides the tabbar if there's only one element in the tab widget." );
	mViewMenu->add( "Line Breaking Column" );

	mViewMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		if ( item->getText() == "Show Line Numbers" ) {
			mConfig.editor.showLineNumbers = item->asType<UIMenuCheckBox>()->isActive();
			mEditorSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setShowLineNumber( mConfig.editor.showLineNumbers );
			} );
		} else if ( item->getText() == "Show White Space" ) {
			mConfig.editor.showWhiteSpaces = item->asType<UIMenuCheckBox>()->isActive();
			mEditorSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setShowWhitespaces( mConfig.editor.showWhiteSpaces );
			} );
		} else if ( item->getText() == "Highlight Matching Bracket" ) {
			mConfig.editor.highlightMatchingBracket = item->asType<UIMenuCheckBox>()->isActive();
			mEditorSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setHighlightMatchingBracket( mConfig.editor.highlightMatchingBracket );
			} );
		} else if ( item->getText() == "Highlight Current Line" ) {
			mConfig.editor.highlightCurrentLine = item->asType<UIMenuCheckBox>()->isActive();
			mEditorSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setHighlightCurrentLine( mConfig.editor.highlightCurrentLine );
			} );
		} else if ( item->getText() == "Highlight Selection Match" ) {
			mConfig.editor.highlightSelectionMatch = item->asType<UIMenuCheckBox>()->isActive();
			mEditorSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setHighlightSelectionMatch( mConfig.editor.highlightSelectionMatch );
			} );
		} else if ( item->getText() == "Enable Horizontal ScrollBar" ) {
			mConfig.editor.horizontalScrollbar = item->asType<UIMenuCheckBox>()->isActive();
			mEditorSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setHorizontalScrollBarEnabled( mConfig.editor.horizontalScrollbar );
			} );
		} else if ( item->getText() == "Enable Color Picker" ) {
			mConfig.editor.colorPickerSelection = item->asType<UIMenuCheckBox>()->isActive();
			mEditorSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setEnableColorPickerOnSelection( mConfig.editor.colorPickerSelection );
			} );
		} else if ( item->getText() == "Enable Auto Complete" ) {
			setAutoComplete( item->asType<UIMenuCheckBox>()->isActive() );
		} else if ( item->getText() == "Enable Linter" ) {
			setLinter( item->asType<UIMenuCheckBox>()->isActive() );
		} else if ( item->getText() == "Enable Color Preview" ) {
			mConfig.editor.colorPreview = item->asType<UIMenuCheckBox>()->isActive();
			mEditorSplitter->forEachEditor( [&]( UICodeEditor* editor ) {
				editor->setEnableColorPickerOnSelection( mConfig.editor.colorPreview );
			} );
		} else if ( item->getText() == "Show Document Info" ) {
			mConfig.editor.showDocInfo = item->asType<UIMenuCheckBox>()->isActive();
			if ( mDocInfo )
				mDocInfo->setVisible( mConfig.editor.showDocInfo );
			if ( mEditorSplitter->getCurEditor() )
				updateDocInfo( mEditorSplitter->getCurEditor()->getDocument() );
		} else if ( item->getText() == "Hide tabbar on single tab" ) {
			mConfig.editor.hideTabBarOnSingleTab = item->asType<UIMenuCheckBox>()->isActive();
			mEditorSplitter->setHideTabBarOnSingleTab( mConfig.editor.hideTabBarOnSingleTab );
		} else if ( item->getText() == "Line Breaking Column" ) {
			UIMessageBox* msgBox =
				UIMessageBox::New( UIMessageBox::INPUT, "Set Line Breaking Column:\n"
														"Set 0 to disable it.\n" );
			msgBox->setTitle( mWindowTitle );
			msgBox->setCloseShortcut( { KEY_ESCAPE, 0 } );
			msgBox->getTextInput()->setAllowOnlyNumbers( true, false );
			msgBox->getTextInput()->setText(
				String::toString( mConfig.editor.lineBreakingColumn ) );
			msgBox->show();
			msgBox->addEventListener( Event::MsgBoxConfirmClick, [&, msgBox]( const Event* ) {
				int val;
				if ( String::fromString( val, msgBox->getTextInput()->getText() ) && val >= 0 ) {
					mConfig.editor.lineBreakingColumn = val;
					mEditorSplitter->forEachEditor(
						[val]( UICodeEditor* editor ) { editor->setLineBreakingColumn( val ); } );
					msgBox->closeWindow();
				}
			} );
			setFocusEditorOnClose( msgBox );
		} else {
			String text = String( event->getNode()->asType<UIMenuItem>()->getText() ).toLower();
			String::replaceAll( text, " ", "-" );
			String::replaceAll( text, "/", "-" );
			runCommand( text );
		}
	} );
	return mViewMenu;
}

void App::setFocusEditorOnClose( UIMessageBox* msgBox ) {
	msgBox->addEventListener( Event::OnClose, [&]( const Event* ) {
		if ( mEditorSplitter && mEditorSplitter->getCurEditor() )
			mEditorSplitter->getCurEditor()->setFocus();
	} );
}

Drawable* App::findIcon( const std::string& name ) {
	UIIcon* icon = mUISceneNode->findIcon( name );
	if ( icon )
		return icon->getSize( mMenuIconSize );
	return nullptr;
}

UIMenu* App::createEditMenu() {
	UIPopUpMenu* menu = UIPopUpMenu::New();
	menu->add( "Undo", findIcon( "undo" ), getKeybind( "undo" ) );
	menu->add( "Redo", findIcon( "redo" ), getKeybind( "redo" ) );
	menu->addSeparator();
	menu->add( "Cut", findIcon( "cut" ), getKeybind( "cut" ) );
	menu->add( "Copy", findIcon( "copy" ), getKeybind( "copy" ) );
	menu->add( "Paste", findIcon( "paste" ), getKeybind( "paste" ) );
	menu->addSeparator();
	menu->add( "Select All", findIcon( "select-all" ), getKeybind( "select-all" ) );
	menu->addSeparator();
	menu->add( "Find/Replace", findIcon( "find-replace" ), getKeybind( "find-replace" ) );
	menu->addSeparator();
	menu->add( "Key Bindings", findIcon( "keybindings" ), getKeybind( "keybindings" ) );
	menu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		String text = String( event->getNode()->asType<UIMenuItem>()->getText() ).toLower();
		if ( "key bindings" == text ) {
			runCommand( "keybindings" );
		} else {
			String::replaceAll( text, " ", "-" );
			String::replaceAll( text, "/", "-" );
			runCommand( text );
		}
	} );
	return menu;
}

static std::vector<std::pair<String::StringBaseType, String::StringBaseType>>
makeAutoClosePairs( const std::string& strPairs ) {
	auto curPairs = String::split( strPairs, ',' );
	std::vector<std::pair<String::StringBaseType, String::StringBaseType>> pairs;
	for ( auto pair : curPairs ) {
		if ( pair.size() == 2 )
			pairs.emplace_back( std::make_pair( pair[0], pair[1] ) );
	}
	return pairs;
}

UIMenu* App::createDocumentMenu() {
	mDocMenu = UIPopUpMenu::New();

	mDocMenu->addCheckBox( "Auto Detect Indent Type", mConfig.editor.autoDetectIndentType )
		->setId( "auto_indent" );

	UIPopUpMenu* tabTypeMenu = UIPopUpMenu::New();
	tabTypeMenu->addRadioButton( "Tabs" )->setId( "tabs" );
	tabTypeMenu->addRadioButton( "Spaces" )->setId( "spaces" );
	mDocMenu->addSubMenu( "Indentation Type", nullptr, tabTypeMenu )->setId( "indent_type" );
	tabTypeMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		const String& text = event->getNode()->asType<UIMenuRadioButton>()->getId();
		if ( mEditorSplitter->getCurEditor() ) {
			TextDocument::IndentType indentType = text == "tabs"
													  ? TextDocument::IndentType::IndentTabs
													  : TextDocument::IndentType::IndentSpaces;
			mEditorSplitter->getCurEditor()->getDocument().setIndentType( indentType );
			mConfig.editor.indentSpaces = indentType == TextDocument::IndentType::IndentSpaces;
		}
	} );

	UIPopUpMenu* indentWidthMenu = UIPopUpMenu::New();
	for ( size_t w = 2; w <= 12; w++ )
		indentWidthMenu
			->addRadioButton( String::toString( w ),
							  mEditorSplitter->getCurEditor() &&
								  mEditorSplitter->getCurEditor()->getDocument().getIndentWidth() ==
									  w )
			->setId( String::format( "indent_width_%zu", w ) )
			->setData( w );
	mDocMenu->addSubMenu( "Indent Width", nullptr, indentWidthMenu )->setId( "indent_width" );
	indentWidthMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( mEditorSplitter->getCurEditor() ) {
			int width = event->getNode()->getData();
			mEditorSplitter->getCurEditor()->getDocument().setIndentWidth( width );
			mConfig.editor.indentWidth = width;
		}
	} );

	UIPopUpMenu* tabWidthMenu = UIPopUpMenu::New();
	for ( size_t w = 2; w <= 12; w++ )
		tabWidthMenu
			->addRadioButton( String::toString( w ),
							  mEditorSplitter->getCurEditor() &&
								  mEditorSplitter->getCurEditor()->getTabWidth() == w )
			->setId( String::format( "tab_width_%zu", w ) )
			->setData( w );
	mDocMenu->addSubMenu( "Tab Width", nullptr, tabWidthMenu )->setId( "tab_width" );
	tabWidthMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( mEditorSplitter->getCurEditor() ) {
			int width = event->getNode()->getData();
			mEditorSplitter->getCurEditor()->setTabWidth( width );
			mConfig.editor.tabWidth = width;
		}
	} );

	UIPopUpMenu* lineEndingsMenu = UIPopUpMenu::New();
	lineEndingsMenu->addRadioButton( "Windows (CR/LF)", mConfig.editor.windowsLineEndings )
		->setId( "windows" );
	lineEndingsMenu->addRadioButton( "Unix (LF)", !mConfig.editor.windowsLineEndings )
		->setId( "unix" );
	mDocMenu->addSubMenu( "Line Endings", nullptr, lineEndingsMenu )->setId( "line_endings" );
	lineEndingsMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		bool winLe = event->getNode()->asType<UIRadioButton>()->getId() == "windows";
		if ( mEditorSplitter->getCurEditor() ) {
			mConfig.editor.windowsLineEndings = winLe;
			mEditorSplitter->getCurEditor()->getDocument().setLineEnding(
				winLe ? TextDocument::LineEnding::CRLF : TextDocument::LineEnding::LF );
			updateDocInfo( mEditorSplitter->getCurEditor()->getDocument() );
		}
	} );

	UIPopUpMenu* bracketsMenu = UIPopUpMenu::New();
	mDocMenu->addSubMenu( "Auto-Close Brackets", nullptr, bracketsMenu );
	auto& closeBrackets = mConfig.editor.autoCloseBrackets;
	auto shouldCloseCb = []( UIMenuItem* ) -> bool { return false; };
	bracketsMenu->addCheckBox( "Brackets ()", closeBrackets.find( '(' ) != std::string::npos )
		->setOnShouldCloseCb( shouldCloseCb )
		->setId( "()" );
	bracketsMenu->addCheckBox( "Curly Brackets {}", closeBrackets.find( '{' ) != std::string::npos )
		->setOnShouldCloseCb( shouldCloseCb )
		->setId( "{}" );
	bracketsMenu
		->addCheckBox( "Square Brackets []", closeBrackets.find( '[' ) != std::string::npos )
		->setOnShouldCloseCb( shouldCloseCb )
		->setId( "[]" );
	bracketsMenu->addCheckBox( "Single Quotes ''", closeBrackets.find( '\'' ) != std::string::npos )
		->setOnShouldCloseCb( shouldCloseCb )
		->setId( "''" );
	bracketsMenu
		->addCheckBox( "Double Quotes \"\"", closeBrackets.find( '"' ) != std::string::npos )
		->setOnShouldCloseCb( shouldCloseCb )
		->setId( "\"\"" );
	bracketsMenu->addCheckBox( "Back Quotes ``", closeBrackets.find( '`' ) != std::string::npos )
		->setOnShouldCloseCb( shouldCloseCb )
		->setId( "``" );
	bracketsMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		std::string id = event->getNode()->getId();
		if ( event->getNode()->isType( UI_TYPE_MENUCHECKBOX ) ) {
			UIMenuCheckBox* item = event->getNode()->asType<UIMenuCheckBox>();
			auto curPairs = String::split( mConfig.editor.autoCloseBrackets, ',' );
			auto found = std::find( curPairs.begin(), curPairs.end(), id );
			if ( item->isActive() ) {
				if ( found == curPairs.end() )
					curPairs.push_back( id );
			} else if ( found != curPairs.end() ) {
				curPairs.erase( found );
			}
			mConfig.editor.autoCloseBrackets = String::join( curPairs, ',' );
			auto pairs = makeAutoClosePairs( mConfig.editor.autoCloseBrackets );
			mEditorSplitter->forEachEditor( [&, pairs]( UICodeEditor* editor ) {
				editor->getDocument().setAutoCloseBrackets( !pairs.empty() );
				editor->getDocument().setAutoCloseBracketsPairs( pairs );
			} );
		}
	} );

	mDocMenu->addSeparator();

	mDocMenu->addCheckBox( "Read Only" )->setId( "read_only" );

	mDocMenu->addCheckBox( "Trim Trailing Whitespaces", mConfig.editor.trimTrailingWhitespaces )
		->setId( "trim_whitespaces" );

	mDocMenu->addCheckBox( "Force New Line at End of File", mConfig.editor.forceNewLineAtEndOfFile )
		->setId( "force_nl" );

	mDocMenu->addCheckBox( "Write Unicode BOM", mConfig.editor.writeUnicodeBOM )
		->setId( "write_bom" );

	mDocMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !mEditorSplitter->getCurEditor() ||
			 event->getNode()->isType( UI_TYPE_MENU_SEPARATOR ) ||
			 event->getNode()->isType( UI_TYPE_MENUSUBMENU ) )
			return;
		const String& id = event->getNode()->getId();
		TextDocument& doc = mEditorSplitter->getCurEditor()->getDocument();

		if ( event->getNode()->isType( UI_TYPE_MENUCHECKBOX ) ) {
			UIMenuCheckBox* item = event->getNode()->asType<UIMenuCheckBox>();
			if ( "auto_indent" == id ) {
				doc.setAutoDetectIndentType( item->isActive() );
				mConfig.editor.autoDetectIndentType = item->isActive();
			} else if ( "trim_whitespaces" == id ) {
				doc.setTrimTrailingWhitespaces( item->isActive() );
				mConfig.editor.trimTrailingWhitespaces = item->isActive();
			} else if ( "force_nl" == id ) {
				doc.setForceNewLineAtEndOfFile( item->isActive() );
				mConfig.editor.forceNewLineAtEndOfFile = item->isActive();
			} else if ( "write_bom" == id ) {
				doc.setBOM( item->isActive() );
				mConfig.editor.writeUnicodeBOM = item->isActive();
			} else if ( "read_only" == id ) {
				mEditorSplitter->getCurEditor()->setLocked( item->isActive() );
			}
		}
	} );
	return mDocMenu;
}

void App::updateDocumentMenu() {
	if ( !mEditorSplitter->getCurEditor() )
		return;

	const TextDocument& doc = mEditorSplitter->getCurEditor()->getDocument();

	mDocMenu->find( "auto_indent" )
		->asType<UIMenuCheckBox>()
		->setActive( doc.getAutoDetectIndentType() );

	auto* curIndent = mDocMenu->find( "indent_width" )
						  ->asType<UIMenuSubMenu>()
						  ->getSubMenu()
						  ->find( String::format( "indent_width_%d", doc.getIndentWidth() ) );
	if ( curIndent )
		curIndent->asType<UIMenuRadioButton>()->setActive( true );

	mDocMenu->find( "indent_type" )
		->asType<UIMenuSubMenu>()
		->getSubMenu()
		->find( doc.getIndentType() == TextDocument::IndentType::IndentTabs ? "tabs" : "spaces" )
		->asType<UIMenuRadioButton>()
		->setActive( true );

	mDocMenu->find( "tab_width" )
		->asType<UIMenuSubMenu>()
		->getSubMenu()
		->find( String::format( "tab_width_%d", mEditorSplitter->getCurEditor()->getTabWidth() ) )
		->asType<UIMenuRadioButton>()
		->setActive( true );

	mDocMenu->find( "trim_whitespaces" )
		->asType<UIMenuCheckBox>()
		->setActive( doc.getTrimTrailingWhitespaces() );

	mDocMenu->find( "force_nl" )
		->asType<UIMenuCheckBox>()
		->setActive( doc.getForceNewLineAtEndOfFile() );

	mDocMenu->find( "write_bom" )->asType<UIMenuCheckBox>()->setActive( doc.getBOM() );

	mDocMenu->find( "line_endings" )
		->asType<UIMenuSubMenu>()
		->getSubMenu()
		->find( doc.getLineEnding() == TextDocument::LineEnding::CRLF ? "windows" : "unix" )
		->asType<UIMenuRadioButton>()
		->setActive( true );

	mDocMenu->find( "read_only" )
		->asType<UIMenuCheckBox>()
		->setActive( mEditorSplitter->getCurEditor()->isLocked() );
}

void App::loadKeybindings() {
	if ( mKeybindings.empty() ) {
		KeyBindings bindings( mWindow->getInput() );
		auto defKeybindings = getDefaultKeybindings();
		IniFile ini( mKeybindingsPath );
		if ( FileSystem::fileExists( mKeybindingsPath ) ) {
			mKeybindings = ini.getKeyMap( "keybindings" );
		} else {
			for ( auto it : defKeybindings )
				ini.setValue( "keybindings", bindings.getShortcutString( it.first ), it.second );
			ini.writeFile();
			mKeybindings = ini.getKeyMap( "keybindings" );
		}
		for ( auto key : mKeybindings )
			mKeybindingsInvert[key.second] = key.first;

		if ( defKeybindings.size() != mKeybindings.size() ) {
			bool added = false;
			for ( auto key : defKeybindings ) {
				auto foundCmd = mKeybindingsInvert.find( key.second );
				auto shortcutStr = bindings.getShortcutString( key.first );
				if ( foundCmd == mKeybindingsInvert.end() &&
					 mKeybindings.find( shortcutStr ) == mKeybindings.end() ) {
					mKeybindings[shortcutStr] = key.second;
					mKeybindingsInvert[key.second] = shortcutStr;
					ini.setValue( "keybindings", shortcutStr, key.second );
					added = true;
				}
			}
			if ( added )
				ini.writeFile();
		}
	}
}

void App::onDocumentStateChanged( UICodeEditor*, TextDocument& ) {
	updateEditorState();
}

void App::onDocumentSelectionChange( UICodeEditor* editor, TextDocument& doc ) {
	onDocumentModified( editor, doc );
}

void App::onDocumentCursorPosChange( UICodeEditor*, TextDocument& doc ) {
	updateDocInfo( doc );
}

void App::updateDocInfo( TextDocument& doc ) {
	if ( mConfig.editor.showDocInfo && mDocInfoText ) {
		mDocInfoText->setText( String::format(
			"line: %lld / %lu  col: %lld    %s", doc.getSelection().start().line() + 1,
			doc.linesCount(), doc.getSelection().start().column(),
			doc.getLineEnding() == TextDocument::LineEnding::LF ? "LF" : "CRLF" ) );
	}
}

void App::onCodeEditorFocusChange( UICodeEditor* editor ) {
	updateDocInfo( editor->getDocument() );
	updateDocumentMenu();

	if ( mSearchState.editor && mSearchState.editor != editor ) {
		String word = mSearchState.editor->getHighlightWord();
		mSearchState.editor->setHighlightWord( "" );
		mSearchState.editor->setHighlightTextRange( TextRange() );
		mSearchState.text = "";
		mSearchState.range = TextRange();
		if ( editor ) {
			mSearchState.editor = editor;
			mSearchState.editor->setHighlightWord( word );
			mSearchState.range = TextRange();
		}
	}
}

void App::onColorSchemeChanged( const std::string& ) {
	updateColorSchemeMenu();

	mGlobalSearchTree->updateColorScheme( mEditorSplitter->getCurrentColorScheme() );
}

void App::onDocumentLoaded( UICodeEditor* editor, const std::string& path ) {
	updateEditorTitle( editor );
	if ( editor == mEditorSplitter->getCurEditor() )
		updateCurrentFiletype();
	mEditorSplitter->removeUnusedTab( mEditorSplitter->tabWidgetFromEditor( editor ) );
	auto found = std::find( mRecentFiles.begin(), mRecentFiles.end(), path );
	if ( found != mRecentFiles.end() )
		mRecentFiles.erase( found );
	mRecentFiles.insert( mRecentFiles.begin(), path );
	if ( mRecentFiles.size() > 10 )
		mRecentFiles.resize( 10 );
	updateRecentFiles();
	if ( mEditorSplitter->getCurEditor() == editor ) {
		updateDocumentMenu();
		updateDocInfo( editor->getDocument() );
	}

	if ( !path.empty() ) {
		UITab* tab = reinterpret_cast<UITab*>( editor->getData() );
		tab->setTooltipText( path );
	}

	TextDocument& doc = editor->getDocument();

	if ( mFileWatcher && doc.hasFilepath() &&
		 ( !mDirTree || !mDirTree->isDirInTree( doc.getFileInfo().getFilepath() ) ) ) {
		std::string dir( FileSystem::fileRemoveFileName( doc.getFileInfo().getFilepath() ) );
		mFilesFolderWatches[dir] = mFileWatcher->addWatch( dir, mFileSystemListener );
	}
}

const CodeEditorConfig& App::getCodeEditorConfig() const {
	return mConfig.editor;
}

std::map<KeyBindings::Shortcut, std::string> App::getDefaultKeybindings() {
	auto bindings = UICodeEditorSplitter::getDefaultKeybindings();
	auto local = getLocalKeybindings();
	local.insert( bindings.begin(), bindings.end() );
	return local;
}

std::map<KeyBindings::Shortcut, std::string> App::getLocalKeybindings() {
	return { { { KEY_RETURN, KEYMOD_LALT }, "fullscreen-toggle" },
			 { { KEY_F3, KEYMOD_NONE }, "repeat-find" },
			 { { KEY_F12, KEYMOD_NONE }, "console-toggle" },
			 { { KEY_F, KEYMOD_CTRL }, "find-replace" },
			 { { KEY_Q, KEYMOD_CTRL }, "close-app" },
			 { { KEY_O, KEYMOD_CTRL }, "open-file" },
			 { { KEY_O, KEYMOD_CTRL | KEYMOD_SHIFT }, "open-folder" },
			 { { KEY_F6, KEYMOD_NONE }, "debug-draw-highlight-toggle" },
			 { { KEY_F7, KEYMOD_NONE }, "debug-draw-boxes-toggle" },
			 { { KEY_F8, KEYMOD_NONE }, "debug-draw-debug-data" },
			 { { KEY_K, KEYMOD_CTRL }, "open-locatebar" },
			 { { KEY_F, KEYMOD_CTRL | KEYMOD_SHIFT }, "open-global-search" },
			 { { KEY_L, KEYMOD_CTRL }, "go-to-line" },
			 { { KEY_M, KEYMOD_CTRL }, "menu-toggle" },
			 { { KEY_S, KEYMOD_CTRL | KEYMOD_SHIFT }, "save-all" },
			 { { KEY_F9, KEYMOD_LALT }, "switch-side-panel" } };
}

std::vector<std::string> App::getUnlockedCommands() {
	return { "fullscreen-toggle",  "open-file",	  "open-folder",
			 "console-toggle",	   "close-app",	  "open-locatebar",
			 "open-global-search", "menu-toggle", "switch-side-panel" };
}

void App::closeEditors() {
	removeFolderWatches();
	mConfig.saveProject( mCurrentProject, mEditorSplitter, mConfigPath );
	std::vector<UICodeEditor*> editors = mEditorSplitter->getAllEditors();
	for ( auto editor : editors ) {
		UITabWidget* tabWidget = mEditorSplitter->tabWidgetFromEditor( editor );
		tabWidget->removeTab( (UITab*)editor->getData() );
	}
	mCurrentProject = "";
}

void App::closeFolder() {
	if ( mCurrentProject.empty() )
		return;

	if ( mEditorSplitter->isAnyEditorDirty() ) {
		UIMessageBox* msgBox = UIMessageBox::New(
			UIMessageBox::OK_CANCEL,
			"Do you really want to close the folder?\nSome files haven't been saved." );
		msgBox->addEventListener( Event::MsgBoxConfirmClick,
								  [&]( const Event* ) { closeEditors(); } );
		msgBox->addEventListener( Event::OnClose, [&]( const Event* ) { msgBox = nullptr; } );
		msgBox->setTitle( "Close Folder?" );
		msgBox->center();
		msgBox->show();
	} else {
		closeEditors();
	}
}

void App::createDocAlert( UICodeEditor* editor ) {
	UILinearLayout* docAlert = editor->findByClass<UILinearLayout>( "doc_alert" );

	if ( docAlert )
		return;

	const std::string& msg = R"xml(
	<hbox class="doc_alert" layout_width="wrap_content" layout_height="wrap_content" layout_gravity="top|right">
		<TextView id="doc_alert_text" layout_width="wrap_content" layout_height="wrap_content" margin-right="24dp"
			text="The file on the disk is more recent that the current buffer.&#xA;Do you want to reload it?"
		/>
		<PushButton id="file_reload" layout_width="wrap_content" layout_height="18dp" text="Reload" margin-right="4dp"
					tooltip="Reload the file from disk. Unsaved changes will be lost." />
		<PushButton id="file_overwrite" layout_width="wrap_content" layout_height="18dp" text="Overwrite" margin-right="4dp"
					tooltip="Writes the local changes on disk, overwriting the disk changes" />
		<PushButton id="file_ignore" layout_width="wrap_content" layout_height="18dp" text="Ignore"
					tooltip="Ignores the changes on disk without any action." />
	</hbox>
	)xml";
	docAlert = static_cast<UILinearLayout*>( mUISceneNode->loadLayoutFromString( msg, editor ) );

	editor->enableReportSizeChangeToChilds();

	docAlert->find( "file_reload" )
		->addEventListener( Event::MouseClick, [editor, docAlert]( const Event* event ) {
			const MouseEvent* mouseEvent = static_cast<const MouseEvent*>( event );
			if ( mouseEvent->getFlags() & EE_BUTTON_LMASK ) {
				editor->getDocument().reload();
				editor->disableReportSizeChangeToChilds();
				docAlert->close();
				editor->setFocus();
			}
		} );

	docAlert->find( "file_overwrite" )
		->addEventListener( Event::MouseClick, [editor, docAlert]( const Event* event ) {
			const MouseEvent* mouseEvent = static_cast<const MouseEvent*>( event );
			if ( mouseEvent->getFlags() & EE_BUTTON_LMASK ) {
				editor->getDocument().save();
				editor->disableReportSizeChangeToChilds();
				docAlert->close();
				editor->setFocus();
			}
		} );

	docAlert->find( "file_ignore" )
		->addEventListener( Event::MouseClick, [docAlert, editor]( const Event* event ) {
			const MouseEvent* mouseEvent = static_cast<const MouseEvent*>( event );
			if ( mouseEvent->getFlags() & EE_BUTTON_LMASK ) {
				editor->disableReportSizeChangeToChilds();
				docAlert->close();
				editor->setFocus();
			}
		} );
}

void App::onCodeEditorCreated( UICodeEditor* editor, TextDocument& doc ) {
	const CodeEditorConfig& config = mConfig.editor;
	editor->setFontSize( config.fontSize.asDp( 0, Sizef(), mUISceneNode->getDPI() ) );
	editor->setEnableColorPickerOnSelection( true );
	editor->setColorScheme( mEditorSplitter->getCurrentColorScheme() );
	editor->setShowLineNumber( config.showLineNumbers );
	editor->setShowWhitespaces( config.showWhiteSpaces );
	editor->setHighlightMatchingBracket( config.highlightMatchingBracket );
	editor->setHorizontalScrollBarEnabled( config.horizontalScrollbar );
	editor->setHighlightCurrentLine( config.highlightCurrentLine );
	editor->setTabWidth( config.tabWidth );
	editor->setLineBreakingColumn( config.lineBreakingColumn );
	editor->setHighlightSelectionMatch( config.highlightSelectionMatch );
	editor->setEnableColorPickerOnSelection( config.colorPickerSelection );
	editor->setColorPreview( config.colorPreview );
	editor->setFont( mFontMono );
	doc.setAutoCloseBrackets( !mConfig.editor.autoCloseBrackets.empty() );
	doc.setAutoCloseBracketsPairs( makeAutoClosePairs( mConfig.editor.autoCloseBrackets ) );
	doc.setAutoDetectIndentType( config.autoDetectIndentType );
	doc.setLineEnding( config.windowsLineEndings ? TextDocument::LineEnding::CRLF
												 : TextDocument::LineEnding::LF );
	doc.setTrimTrailingWhitespaces( config.trimTrailingWhitespaces );
	doc.setIndentType( config.indentSpaces ? TextDocument::IndentType::IndentSpaces
										   : TextDocument::IndentType::IndentTabs );
	doc.setForceNewLineAtEndOfFile( config.forceNewLineAtEndOfFile );
	doc.setIndentWidth( config.indentWidth );
	doc.setBOM( config.writeUnicodeBOM );

	editor->addKeyBinds( getLocalKeybindings() );
	editor->addUnlockedCommands( getUnlockedCommands() );
	doc.setCommand( "save-doc", [&] { saveDoc(); } );
	doc.setCommand( "save-as-doc", [&] { saveFileDialog( mEditorSplitter->getCurEditor() ); } );
	doc.setCommand( "save-all", [&] { saveAll(); } );
	doc.setCommand( "find-replace", [&] { showFindView(); } );
	doc.setCommand( "open-global-search", [&] { showGlobalSearch(); } );
	doc.setCommand( "open-locatebar", [&] { showLocateBar(); } );
	doc.setCommand( "repeat-find", [&] { findNextText( mSearchState ); } );
	doc.setCommand( "close-folder", [&] { closeFolder(); } );
	doc.setCommand( "close-app", [&] { closeApp(); } );
	doc.setCommand( "fullscreen-toggle", [&]() {
		mWindow->toggleFullscreen();
		mWindowMenu->find( "fullscreen-mode" )
			->asType<UIMenuCheckBox>()
			->setActive( !mWindow->isWindowed() );
	} );
	doc.setCommand( "open-file", [&] { openFileDialog(); } );
	doc.setCommand( "open-folder", [&] { openFolderDialog(); } );
	doc.setCommand( "console-toggle", [&] {
		mConsole->toggle();
		bool lock = mConsole->isActive();
		mEditorSplitter->forEachEditor(
			[lock]( UICodeEditor* editor ) { editor->setLocked( lock ); } );
	} );
	doc.setCommand( "lock", [&] {
		if ( mEditorSplitter->getCurEditor() ) {
			mEditorSplitter->getCurEditor()->setLocked( true );
			updateDocumentMenu();
		}
	} );
	doc.setCommand( "unlock", [&] {
		if ( mEditorSplitter->getCurEditor() ) {
			mEditorSplitter->getCurEditor()->setLocked( false );
			updateDocumentMenu();
		}
	} );
	doc.setCommand( "lock-toggle", [&] {
		if ( mEditorSplitter->getCurEditor() ) {
			mEditorSplitter->getCurEditor()->setLocked(
				!mEditorSplitter->getCurEditor()->isLocked() );
			updateDocumentMenu();
		}
	} );
	doc.setCommand( "keybindings",
					[&] { mEditorSplitter->loadFileFromPathInNewTab( mKeybindingsPath ); } );
	doc.setCommand( "debug-draw-boxes-toggle",
					[&] { mUISceneNode->setDrawBoxes( !mUISceneNode->getDrawBoxes() ); } );
	doc.setCommand( "debug-draw-highlight-toggle", [&] {
		mUISceneNode->setHighlightFocus( !mUISceneNode->getHighlightFocus() );
		mUISceneNode->setHighlightOver( !mUISceneNode->getHighlightOver() );
	} );
	doc.setCommand( "debug-draw-debug-data",
					[&] { mUISceneNode->setDrawDebugData( !mUISceneNode->getDrawDebugData() ); } );
	doc.setCommand( "go-to-line", [&] { goToLine(); } );
	doc.setCommand( "load-current-dir", [&] { loadCurrentDirectory(); } );
	doc.setCommand( "menu-toggle", [&] { toggleSettingsMenu(); } );
	doc.setCommand( "switch-side-panel", [&] { switchSidePanel(); } );

	editor->addEventListener( Event::OnDocumentSave, [&]( const Event* event ) {
		UICodeEditor* editor = event->getNode()->asType<UICodeEditor>();
		updateEditorTabTitle( editor );
		if ( mEditorSplitter->getCurEditor() == editor )
			editor->setFocus();
		if ( editor->getDocument().getFilePath() == mKeybindingsPath ) {
			mKeybindings.clear();
			mKeybindingsInvert.clear();
			loadKeybindings();
			mEditorSplitter->forEachEditor( [&]( UICodeEditor* ed ) {
				ed->getKeyBindings().reset();
				ed->getKeyBindings().addKeybindsString( mKeybindings );
			} );
		}
	} );

	editor->addEventListener(
		Event::OnDocumentDirtyOnFileSysten, [&, editor]( const Event* event ) {
			const DocEvent* docEvent = static_cast<const DocEvent*>( event );
			FileInfo file( docEvent->getDoc()->getFileInfo().getFilepath() );
			TextDocument* doc = docEvent->getDoc();
			if ( doc->getFileInfo() != file ) {
				if ( doc->isDirty() ) {
					editor->runOnMainThread( [&, editor]() { createDocAlert( editor ); } );
				} else {
					auto hash = String::hash( docEvent->getDoc()->getFilePath() );
					editor->removeActionsByTag( hash );
					editor->runOnMainThread( [doc]() { doc->reload(); }, Seconds( 0.5f ), hash );
				}
			}
		} );

	if ( !mKeybindings.empty() ) {
		editor->getKeyBindings().reset();
		editor->getKeyBindings().addKeybindsString( mKeybindings );
	}

	editor->addEventListener( Event::OnDocumentClosed, [&]( const Event* event ) {
		if ( !appInstance )
			return;
		const DocEvent* docEvent = static_cast<const DocEvent*>( event );
		std::string dir( FileSystem::fileRemoveFileName( docEvent->getDoc()->getFilePath() ) );
		auto itWatch = mFilesFolderWatches.find( dir );
		if ( mFileWatcher && itWatch != mFilesFolderWatches.end() ) {
			if ( !mDirTree || !mDirTree->isDirInTree( dir ) ) {
				mFileWatcher->removeWatch( itWatch->second );
			}
			mFilesFolderWatches.erase( itWatch );
		}
	} );

	if ( config.autoComplete && !mAutoCompleteModule )
		setAutoComplete( config.autoComplete );

	if ( config.autoComplete && mAutoCompleteModule )
		editor->registerModule( mAutoCompleteModule );

	if ( config.linter && !mLinterModule )
		setLinter( config.linter );

	if ( config.linter && mLinterModule )
		editor->registerModule( mLinterModule );
}

bool App::setAutoComplete( bool enable ) {
	mConfig.editor.autoComplete = enable;
	if ( enable && !mAutoCompleteModule ) {
		mAutoCompleteModule = eeNew( AutoCompleteModule, ( mThreadPool ) );
		mEditorSplitter->forEachEditor(
			[&]( UICodeEditor* editor ) { editor->registerModule( mAutoCompleteModule ); } );
		return true;
	}
	if ( !enable && mAutoCompleteModule )
		eeSAFE_DELETE( mAutoCompleteModule );
	return false;
}

bool App::setLinter( bool enable ) {
	mConfig.editor.linter = enable;
	if ( enable && !mLinterModule ) {
		mLinterModule =
			eeNew( LinterModule, ( mResPath + "assets/linters/linters.json", mThreadPool ) );
		mEditorSplitter->forEachEditor(
			[&]( UICodeEditor* editor ) { editor->registerModule( mLinterModule ); } );
		return true;
	}
	if ( !enable && mLinterModule )
		eeSAFE_DELETE( mLinterModule );
	return false;
}

void App::loadCurrentDirectory() {
	if ( !mEditorSplitter->getCurEditor() )
		return;
	std::string path( mEditorSplitter->getCurEditor()->getDocument().getFilePath() );
	if ( path.empty() )
		return;
	path = FileSystem::fileRemoveFileName( path );
	if ( !FileSystem::isDirectory( path ) )
		return;
	loadFolder( path );
}

UIPopUpMenu* App::createToolsMenu() {
	mToolsMenu = UIPopUpMenu::New();
	mToolsMenu->add( "Locate...", findIcon( "search" ), getKeybind( "open-locatebar" ) );
	mToolsMenu->add( "Project Find...", findIcon( "search" ), getKeybind( "open-global-search" ) );
	mToolsMenu->add( "Go to line...", findIcon( "go-to-line" ), getKeybind( "go-to-line" ) );
	mToolsMenu->add( "Load current document directory as folder", findIcon( "folder" ),
					 getKeybind( "load-current-dir" ) );
	mToolsMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		std::string txt( item->getText() );
		if ( txt == "Locate..." ) {
			showLocateBar();
		} else if ( txt == "Project Find..." ) {
			showGlobalSearch();
		} else if ( txt == "Go to line..." ) {
			goToLine();
		} else if ( txt == "Load current document directory as folder" ) {
			loadCurrentDirectory();
		}
	} );
	return mToolsMenu;
}

void App::toggleSettingsMenu() {
	if ( ( !mSettingsMenu->isVisible() || mSettingsMenu->isHiding() ) &&
		 mSettingsMenu->getInactiveTime().getElapsedTime().asMilliseconds() > 1 ) {
		Vector2f pos( mSettingsButton->getPixelsPosition() );
		mSettingsButton->nodeToWorldTranslation( pos );
		UIMenu::findBestMenuPos( pos, mSettingsMenu );
		mSettingsMenu->setPixelsPosition( pos );
		mSettingsMenu->show();
	} else {
		mSettingsMenu->hide();
	}
}

void App::createSettingsMenu() {
	mSettingsMenu = UIPopUpMenu::New();
	mSettingsMenu->add( "New", findIcon( "document-new" ), getKeybind( "create-new" ) );
	mSettingsMenu->add( "Open File...", findIcon( "document-open" ), getKeybind( "open-file" ) );
	mSettingsMenu->add( "Open Folder...", findIcon( "document-open" ),
						getKeybind( "open-folder" ) );
	mSettingsMenu->addSubMenu( "Recent Files", findIcon( "document-recent" ), UIPopUpMenu::New() );
	mSettingsMenu->addSubMenu( "Recent Folders", findIcon( "document-recent" ),
							   UIPopUpMenu::New() );
	mSettingsMenu->addSeparator();
	mSettingsMenu->add( "Save", findIcon( "document-save" ), getKeybind( "save-doc" ) );
	mSettingsMenu->add( "Save as...", findIcon( "document-save-as" ), getKeybind( "save-as-doc" ) );
	mSettingsMenu->add( "Save All", findIcon( "document-save-as" ), getKeybind( "save-all" ) );
	mSettingsMenu->addSeparator();
	mSettingsMenu->addSubMenu( "Filetype", nullptr, createFiletypeMenu() );
	mSettingsMenu->addSubMenu( "Color Scheme", nullptr, createColorSchemeMenu() );
	mSettingsMenu->addSubMenu( "Document", nullptr, createDocumentMenu() );
	mSettingsMenu->addSubMenu( "Edit", nullptr, createEditMenu() );
	mSettingsMenu->addSubMenu( "View", nullptr, createViewMenu() );
	mSettingsMenu->addSubMenu( "Tools", nullptr, createToolsMenu() );
	mSettingsMenu->addSubMenu( "Window", nullptr, createWindowMenu() );
	mSettingsMenu->addSeparator();
	mSettingsMenu->add( "Close", findIcon( "document-close" ), getKeybind( "close-doc" ) );
	mSettingsMenu->add( "Close Folder", findIcon( "document-close" ),
						getKeybind( "close-folder" ) );
	mSettingsMenu->addSeparator();
	mSettingsMenu->add( "Quit", findIcon( "quit" ), getKeybind( "close-app" ) );
	mSettingsButton = mUISceneNode->find<UITextView>( "settings" );
	mSettingsButton->addEventListener( Event::MouseClick,
									   [&]( const Event* ) { toggleSettingsMenu(); } );
	mSettingsMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		const String& name = event->getNode()->asType<UIMenuItem>()->getText();
		if ( name == "New" ) {
			runCommand( "create-new" );
		} else if ( name == "Open File..." ) {
			runCommand( "open-file" );
		} else if ( name == "Open Folder..." ) {
			runCommand( "open-folder" );
		} else if ( name == "Save" ) {
			runCommand( "save-doc" );
		} else if ( name == "Save as..." ) {
			runCommand( "save-as-doc" );
		} else if ( name == "Save All" ) {
			runCommand( "save-all" );
		} else if ( name == "Close" ) {
			runCommand( "close-doc" );
		} else if ( name == "Close Folder" ) {
			runCommand( "close-folder" );
		} else if ( name == "Quit" ) {
			runCommand( "close-app" );
		}
	} );
	updateRecentFiles();
	updateRecentFolders();
}

void App::updateColorSchemeMenu() {
	for ( size_t i = 0; i < mColorSchemeMenu->getCount(); i++ ) {
		UIMenuRadioButton* menuItem = mColorSchemeMenu->getItem( i )->asType<UIMenuRadioButton>();
		menuItem->setActive( mEditorSplitter->getCurrentColorSchemeName() == menuItem->getText() );
	}
}

UIMenu* App::createColorSchemeMenu() {
	mColorSchemeMenu = UIPopUpMenu::New();
	for ( auto& colorScheme : mEditorSplitter->getColorSchemes() ) {
		mColorSchemeMenu->addRadioButton(
			colorScheme.first, mEditorSplitter->getCurrentColorSchemeName() == colorScheme.first );
	}
	mColorSchemeMenu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		const String& name = item->getText();
		mEditorSplitter->setColorScheme( name );
	} );
	return mColorSchemeMenu;
}

UIMenu* App::createFiletypeMenu() {
	auto* dM = SyntaxDefinitionManager::instance();
	mFiletypeMenu = UIPopUpMenu::New();
	auto names = dM->getLanguageNames();
	for ( auto& name : names ) {
		mFiletypeMenu->addRadioButton(
			name,
			mEditorSplitter->getCurEditor() &&
				mEditorSplitter->getCurEditor()->getSyntaxDefinition().getLanguageName() == name );
	}
	mFiletypeMenu->addEventListener( Event::OnItemClicked, [&, dM]( const Event* event ) {
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		const String& name = item->getText();
		if ( mEditorSplitter->getCurEditor() ) {
			mEditorSplitter->getCurEditor()->setSyntaxDefinition(
				dM->getStyleByLanguageName( name ) );
			updateCurrentFiletype();
		}
	} );
	return mFiletypeMenu;
}

void App::updateCurrentFiletype() {
	if ( !mEditorSplitter->getCurEditor() )
		return;
	std::string curLang( mEditorSplitter->getCurEditor()->getSyntaxDefinition().getLanguageName() );
	for ( size_t i = 0; i < mFiletypeMenu->getCount(); i++ ) {
		UIMenuRadioButton* menuItem = mFiletypeMenu->getItem( i )->asType<UIMenuRadioButton>();
		std::string itemLang( menuItem->getText() );
		menuItem->setActive( curLang == itemLang );
	}
}

void App::updateEditorState() {
	if ( mEditorSplitter->getCurEditor() ) {
		updateEditorTitle( mEditorSplitter->getCurEditor() );
		updateCurrentFiletype();
		updateDocumentMenu();
	}
}

void App::removeFolderWatches() {
	if ( mFileWatcher ) {
		for ( auto dir : mFolderWatches )
			mFileWatcher->removeWatch( dir );
		mFolderWatches.clear();

		for ( auto fileFolder : mFilesFolderWatches )
			mFileWatcher->removeWatch( fileFolder.second );
		mFilesFolderWatches.clear();
	}
}

void App::loadDirTree( const std::string& path ) {
	Clock* clock = eeNew( Clock, () );
	mDirTree = std::make_unique<ProjectDirectoryTree>( path, mThreadPool );
	Log::info( "Loading DirTree: %s", path.c_str() );
	mDirTree->scan(
		[&, clock]( ProjectDirectoryTree& dirTree ) {
			Log::info( "DirTree read in: %.2fms. Found %ld files.",
					   clock->getElapsedTime().asMilliseconds(), dirTree.getFilesCount() );
			eeDelete( clock );
			mDirTreeReady = true;
			mUISceneNode->runOnMainThread( [&] { updateLocateTable(); } );
			if ( mFileWatcher ) {
				removeFolderWatches();
				auto newDirs = dirTree.getDirectories();
				for ( auto dir : newDirs )
					mFolderWatches.insert( mFileWatcher->addWatch( dir, mFileSystemListener ) );
			}
		},
		SyntaxDefinitionManager::instance()->getExtensionsPatternsSupported() );
}

void App::initProjectTreeView( const std::string& path ) {
	mProjectTreeView = mUISceneNode->find<UITreeView>( "project_view" );
	mProjectTreeView->setColumnsHidden(
		{ FileSystemModel::Icon, FileSystemModel::Size, FileSystemModel::Group,
		  FileSystemModel::Inode, FileSystemModel::Owner, FileSystemModel::SymlinkTarget,
		  FileSystemModel::Permissions, FileSystemModel::ModificationTime, FileSystemModel::Path },
		true );
	mProjectTreeView->setIconSize( mMenuIconSize );
	mProjectTreeView->setExpanderIconSize( mMenuIconSize );
	mProjectTreeView->setExpandedIcon( "folder-open" );
	mProjectTreeView->setContractedIcon( "folder" );
	mProjectTreeView->setHeadersVisible( false );
	mProjectTreeView->setExpandersAsIcons( true );
	mProjectTreeView->addEventListener( Event::OnModelEvent, [&]( const Event* event ) {
		const ModelEvent* modelEvent = static_cast<const ModelEvent*>( event );
		if ( modelEvent->getModelEventType() == ModelEventType::Open ) {
			Variant vPath(
				modelEvent->getModel()->data( modelEvent->getModelIndex(), Model::Role::Custom ) );
			if ( vPath.isValid() && vPath.is( Variant::Type::cstr ) ) {
				std::string path( vPath.asCStr() );
				UITab* tab = mEditorSplitter->isDocumentOpen( path );
				if ( !tab ) {
					FileInfo fileInfo( path );
					if ( fileInfo.exists() && fileInfo.isRegularFile() )
						mEditorSplitter->loadFileFromPathInNewTab( path );
				} else {
					tab->getTabWidget()->setTabSelected( tab );
				}
			}
		}
	} );
	mProjectTreeView->addEventListener( Event::KeyDown, [&]( const Event* event ) {
		const KeyEvent* keyEvent = static_cast<const KeyEvent*>( event );
		if ( mEditorSplitter->getCurEditor() ) {
			std::string cmd =
				mEditorSplitter->getCurEditor()->getKeyBindings().getCommandFromKeyBind(
					{ keyEvent->getKeyCode(), keyEvent->getMod() } );
			if ( !cmd.empty() && mEditorSplitter->getCurEditor()->isUnlockedCommand( cmd ) ) {
				mEditorSplitter->getCurEditor()->getDocument().execute( cmd );
			}
		}
	} );

	if ( !path.empty() && FileSystem::fileExists( path ) ) {
		if ( FileSystem::isDirectory( path ) ) {
			loadFolder( path );
		} else {
			std::string rpath( FileSystem::getRealPath( path ) );

			mProjectTreeView->setModel( FileSystemModel::New(
				FileSystem::fileRemoveFileName( rpath ), FileSystemModel::Mode::FilesAndDirectories,
				{ true, true, true } ) );

			mEditorSplitter->loadFileFromPath( rpath );
		}
	} else {
		loadFolder( "." );
	}

	mProjectTreeView->setAutoExpandOnSingleColumn( true );
}

void App::loadFolder( const std::string& path ) {
	if ( !mCurrentProject.empty() )
		closeEditors();

	std::string rpath( FileSystem::getRealPath( path ) );
	mCurrentProject = rpath;
	loadDirTree( rpath );

	mConfig.loadProject( rpath, mEditorSplitter, mConfigPath );

	mProjectTreeView->setModel( FileSystemModel::New(
		rpath, FileSystemModel::Mode::FilesAndDirectories, { true, true, true } ) );

	auto found = std::find( mRecentFolders.begin(), mRecentFolders.end(), rpath );
	if ( found != mRecentFolders.end() )
		mRecentFolders.erase( found );
	mRecentFolders.insert( mRecentFolders.begin(), rpath );
	if ( mRecentFolders.size() > 10 )
		mRecentFolders.resize( 10 );

	updateRecentFolders();

	if ( mEditorSplitter->getCurEditor() )
		mEditorSplitter->getCurEditor()->setFocus();
}

FontTrueType* App::loadFont( const std::string& name, std::string fontPath,
							 const std::string& fallback ) {
	if ( fontPath.empty() || !FileSystem::fileExists( fontPath ) )
		fontPath = fallback;
	if ( isRelativePath( fontPath ) )
		fontPath = mResPath + fontPath;
	return FontTrueType::New( name, fontPath );
}

void App::init( const std::string& file, const Float& pidelDensity ) {
	DisplayManager* displayManager = Engine::instance()->getDisplayManager();
	Display* currentDisplay = displayManager->getDisplayIndex( 0 );
	mDisplayDPI = currentDisplay->getDPI();
	mResPath = Sys::getProcessPath();

	loadConfig();

	mConfig.window.pixelDensity =
		pidelDensity > 0 ? pidelDensity
						 : ( mConfig.window.pixelDensity > 0 ? mConfig.window.pixelDensity
															 : currentDisplay->getPixelDensity() );

	displayManager->enableScreenSaver();
	displayManager->enableMouseFocusClickThrough();
	displayManager->disableBypassCompositor();

	Engine* engine = Engine::instance();

	WindowSettings winSettings = engine->createWindowSettings( &mConfig.iniState, "window" );
	winSettings.PixelDensity = 1;
	winSettings.Width = mConfig.window.size.getWidth();
	winSettings.Height = mConfig.window.size.getHeight();
	if ( winSettings.Icon.empty() )
		winSettings.Icon = mConfig.window.winIcon;
	ContextSettings contextSettings = engine->createContextSettings( &mConfig.ini, "window" );
	mWindow = engine->createWindow( winSettings, contextSettings );

	if ( mWindow->isOpen() ) {
		loadKeybindings();

		PixelDensity::setPixelDensity( mConfig.window.pixelDensity );

		if ( mConfig.window.maximized )
			mWindow->maximize();

		mWindow->setCloseRequestCallback(
			[&]( EE::Window::Window* win ) -> bool { return onCloseRequestCallback( win ); } );

		mWindow->getInput()->pushCallback( [&]( InputEvent* event ) {
			if ( event->Type == InputEvent::FileDropped ) {
				onFileDropped( event->file.file );
			} else if ( event->Type == InputEvent::TextDropped ) {
				onTextDropped( event->textdrop.text );
			}
		} );

		PixelDensity::setPixelDensity( eemax( mWindow->getScale(), mConfig.window.pixelDensity ) );

		mUISceneNode = UISceneNode::New();

		mFont = loadFont( "sans-serif", mConfig.ui.serifFont, "assets/fonts/NotoSans-Regular.ttf" );
		mFontMono =
			loadFont( "monospace", mConfig.ui.monospaceFont, "assets/fonts/DejaVuSansMono.ttf" );
		if ( mFontMono )
			mFontMono->setBoldAdvanceSameAsRegular( true );

		FontTrueType* iconFont =
			FontTrueType::New( "icon", mResPath + "assets/fonts/remixicon.ttf" );

		if ( !mFont || !mFontMono || !iconFont ) {
			printf( "Font not found!" );
			return;
		}

		SceneManager::instance()->add( mUISceneNode );

		UITheme* theme =
			UITheme::load( "uitheme", "uitheme", "", mFont, mResPath + "assets/ui/breeze.css" );
		theme->setDefaultFontSize( mConfig.ui.fontSize.asDp( 0, Sizef(), mDisplayDPI ) );
		mUISceneNode->setStyleSheet( theme->getStyleSheet() );
		mUISceneNode
			->getUIThemeManager()
			//->setDefaultEffectsEnabled( true )
			->setDefaultTheme( theme )
			->setDefaultFont( mFont )
			->setDefaultFontSize( mConfig.ui.fontSize.asDp( 0, Sizef(), mDisplayDPI ) )
			->add( theme );

		mUISceneNode->getRoot()->addClass( "appbackground" );

		const std::string baseUI = R"xml(
		<style>
		TextInput#search_find,
		TextInput#search_replace,
		TextInput#locate_find,
		TextInput#global_search_find {
			padding-top: 0;
			padding-bottom: 0;
		}
		#search_bar,
		#global_search_bar,
		#locate_bar {
			padding-left: 4dp;
			padding-right: 4dp;
			padding-bottom: 3dp;
			margin-bottom: 2dp;
			margin-top: 2dp;
		}
		.close_button {
			width: 12dp;
			height: 12dp;
			border-radius: 6dp;
			background-color: var(--icon-back-hover);
			foreground-image: poly(line, var(--icon-line-hover), "0dp 0dp, 6dp 6dp"), poly(line, var(--icon-line-hover), "6dp 0dp, 0dp 6dp");
			foreground-position: 3dp 3dp, 3dp 3dp;
			transition: all 0.15s;
		}
		.close_button:hover {
			background-color: var(--icon-back-alert);
		}
		#settings {
			color: #eff0f188;
			font-family: icon;
			font-size: 16dp;
			margin-top: 6dp;
			margin-right: 22dp;
			transition: all 0.15s;
		}
		#settings:hover {
			color: var(--primary);
		}
		#doc_info {
			background-color: var(--back);
			margin-bottom: 22dp;
			margin-right: 22dp;
			border-radius: 8dp;
			padding: 6dp;
			opacity: 0.8;
		}
		#doc_info > TextView {
			color: var(--font);
		}
		#search_find.error,
		#search_replace.error {
			border-color: #ff4040;
		}
		TableView#locate_bar_table > tableview::row > tableview::cell:nth-child(2) {
			color: var(--font-hint);
		}
		TableView#locate_bar_table > tableview::row:selected > tableview::cell:nth-child(2) {
			color: var(--font);
		}
		#search_tree treeview::cell {
			font-family: monospace;
		}
		#global_search_history {
			padding-top: 0dp;
			padding-bottom: 0dp;
		}
		.doc_alert {
			padding: 16dp;
			border-width: 2dp;
			border-radius: 4dp;
			border-color: var(--primary);
			background-color: var(--back);
			margin-right: 24dp;
			margin-top: 24dp;
			cursor: arrow;
		}
		</style>
		<RelativeLayout id="main_layout" layout_width="match_parent" layout_height="match_parent">
		<Splitter id="project_splitter" layout_width="match_parent" layout_height="match_parent">
			<TabWidget id="panel" tabbar-hide-on-single-tab="true" tabbar-allow-rearrange="true">
				<TreeView id="project_view" />
				<Tab text="Project" owns="project_view" />
			</TabWidget>
			<vbox>
				<RelativeLayout layout_width="match_parent" layout_height="0" layout_weight="1">
					<vbox id="code_container" layout_width="match_parent" layout_height="match_parent"></vbox>
					<hbox id="doc_info" layout_width="wrap_content" layout_height="wrap_content" layout_gravity="bottom|right" enabled="false">
						<TextView id="doc_info_text" layout_width="wrap_content" layout_height="wrap_content" />
					</hbox>
				</RelativeLayout>
				<searchbar id="search_bar" layout_width="match_parent" layout_height="wrap_content">
					<vbox layout_width="wrap_content" layout_height="wrap_content" margin-right="4dp">
						<TextView layout_width="wrap_content" layout_height="18dp" text="Find:" margin-bottom="2dp" />
						<TextView layout_width="wrap_content" layout_height="18dp" text="Replace with:" />
					</vbox>
					<vbox layout_width="0" layout_weight="1" layout_height="wrap_content" margin-right="4dp">
						<TextInput id="search_find" layout_width="match_parent" layout_height="18dp" padding="0" margin-bottom="2dp" />
						<TextInput id="search_replace" layout_width="match_parent" layout_height="18dp" padding="0" />
					</vbox>
					<vbox layout_width="wrap_content" layout_height="wrap_content" margin-right="4dp">
						<CheckBox id="case_sensitive" layout_width="wrap_content" layout_height="wrap_content" text="Case sensitive" selected="true" />
						<CheckBox id="whole_word" layout_width="wrap_content" layout_height="wrap_content" text="Match Whole Word" selected="false" />
						<CheckBox id="lua_pattern" layout_width="wrap_content" layout_height="wrap_content" text="Lua Pattern" selected="false" />
					</vbox>
					<vbox layout_width="wrap_content" layout_height="wrap_content">
						<hbox layout_width="wrap_content" layout_height="wrap_content" margin-bottom="2dp">
							<PushButton id="find_prev" layout_width="wrap_content" layout_height="18dp" text="Previous" margin-right="4dp" />
							<PushButton id="find_next" layout_width="wrap_content" layout_height="18dp" text="Next" margin-right="4dp" />
							<RelativeLayout layout_width="0" layout_weight="1" layout_height="18dp">
								<Widget id="searchbar_close" class="close_button" layout_width="wrap_content" layout_height="wrap_content" layout_gravity="center_vertical|right" margin-right="2dp" />
							</RelativeLayout>
						</hbox>
						<hbox layout_width="wrap_content" layout_height="wrap_content">
							<PushButton id="replace" layout_width="wrap_content" layout_height="18dp" text="Replace" margin-right="4dp" />
							<PushButton id="replace_find" layout_width="wrap_content" layout_height="18dp" text="Replace & Find" margin-right="4dp" />
							<PushButton id="replace_all" layout_width="wrap_content" layout_height="18dp" text="Replace All" />
						</hbox>
					</vbox>
				</searchbar>
				<locatebar id="locate_bar" layout_width="match_parent" layout_height="wrap_content" visible="false">
					<TextInput id="locate_find" layout_width="0" layout_weight="1" layout_height="18dp" padding="0" margin-bottom="2dp" margin-right="4dp" hint="Search files by name ( append `l ` to go to line )" />
					<Widget id="locatebar_close" class="close_button" layout_width="wrap_content" layout_height="wrap_content" layout_gravity="center_vertical|right"/>
				</locatebar>
				<globalsearchbar id="global_search_bar" layout_width="match_parent" layout_height="wrap_content">
					<hbox layout_width="match_parent" layout_height="wrap_content">
						<TextView layout_width="wrap_content" layout_height="wrap_content" text="Search for:" margin-right="4dp" />
						<vbox layout_width="0" layout_weight="1" layout_height="wrap_content">
							<TextInput id="global_search_find" layout_width="match_parent" layout_height="wrap_content" layout_height="18dp" padding="0" margin-bottom="2dp" />
							<hbox layout_width="match_parent" layout_height="wrap_content">
								<CheckBox id="case_sensitive" layout_width="wrap_content" layout_height="wrap_content" text="Case sensitive" selected="true" />
								<CheckBox id="whole_word" layout_width="wrap_content" layout_height="wrap_content" text="Match Whole Word" selected="false" margin-left="8dp" />
								<Widget layout_width="0" layout_weight="1" layout_height="match_parent" />
								<TextView layout_width="wrap_content" layout_height="wrap_content" text="History:" margin-right="4dp" layout_height="18dp" />
								<DropDownList id="global_search_history" layout_width="300dp" layout_height="18dp" margin-right="4dp" />
								<PushButton id="global_search" layout_width="wrap_content" layout_height="18dp" text="Search" />
							</hbox>
						</vbox>
						<Widget id="global_searchbar_close" class="close_button" layout_width="wrap_content" layout_height="wrap_content" layout_gravity="top|right" margin-left="4dp" margin-top="4dp" />
					</hbox>
				</globalsearchbar>
			</vbox>
		</Splitter>
		<TextView id="settings" layout_width="wrap_content" layout_height="wrap_content" text="&#xf0e9;" layout_gravity="top|right" />
		</RelativeLayout>
		)xml";

		UIIconTheme* iconTheme = UIIconTheme::New( "remixicon" );
		mMenuIconSize = mConfig.ui.fontSize.asPixels( 0, Sizef(), mDisplayDPI );
		std::unordered_map<std::string, Uint32> icons = {
			{ "document-new", 0xecc3 },
			{ "document-open", 0xed70 },
			{ "document-save", 0xf0b3 },
			{ "document-save-as", 0xf0b3 },
			{ "document-close", 0xeb99 },
			{ "quit", 0xeb97 },
			{ "undo", 0xea58 },
			{ "redo", 0xea5a },
			{ "cut", 0xf0c1 },
			{ "copy", 0xecd5 },
			{ "paste", 0xeb91 },
			{ "split-horizontal", 0xf17a },
			{ "split-vertical", 0xf17b },
			{ "find-replace", 0xed2b },
			{ "folder", 0xed54 },
			{ "folder-open", 0xed70 },
			{ "folder-add", 0xed5a },
			{ "file", 0xecc3 },
			{ "file-code", 0xecd1 },
			{ "file-edit", 0xecdb },
			{ "font-size", 0xed8d },
			{ "zoom-in", 0xf2db },
			{ "zoom-out", 0xf2dd },
			{ "zoom-reset", 0xeb47 },
			{ "fullscreen", 0xed9c },
			{ "keybindings", 0xee75 },
			{ "tree-expanded", 0xea50 },
			{ "tree-contracted", 0xea54 },
			{ "search", 0xf0d1 },
			{ "go-up", 0xea78 },
			{ "ok", 0xeb7a },
			{ "cancel", 0xeb98 },
			{ "color-picker", 0xf13d },
			{ "pixel-density", 0xed8c },
			{ "go-to-line", 0xf1f8 },
			{ "table-view", 0xf1de },
			{ "list-view", 0xecf1 },
		};
		for ( auto icon : icons )
			iconTheme->add( UIGlyphIcon::New( icon.first, iconFont, icon.second ) );

		mUISceneNode->getUIIconThemeManager()->setCurrentTheme( iconTheme );

		UIWidgetCreator::registerWidget( "searchbar", UISearchBar::New );
		UIWidgetCreator::registerWidget( "locatebar", UILocateBar::New );
		UIWidgetCreator::registerWidget( "globalsearchbar", UIGlobalSearchBar::New );
		mUISceneNode->loadLayoutFromString( baseUI );
		mUISceneNode->bind( "main_layout", mMainLayout );
		mUISceneNode->bind( "code_container", mBaseLayout );
		mUISceneNode->bind( "search_bar", mSearchBarLayout );
		mUISceneNode->bind( "global_search_bar", mGlobalSearchBarLayout );
		mUISceneNode->bind( "locate_bar", mLocateBarLayout );
		mUISceneNode->bind( "doc_info", mDocInfo );
		mUISceneNode->bind( "doc_info_text", mDocInfoText );
		mUISceneNode->bind( "panel", mSidePanel );
		mUISceneNode->bind( "project_splitter", mProjectSplitter );
		mUISceneNode->bind( "locate_find", mLocateInput );
		mUISceneNode->addEventListener( Event::KeyDown, [&]( const Event* event ) {
			trySendUnlockedCmd( *static_cast<const KeyEvent*>( event ) );
		} );
		mDocInfo->setVisible( mConfig.editor.showDocInfo );
		mSearchBarLayout->setVisible( false )->setEnabled( false );
		mGlobalSearchBarLayout->setVisible( false )->setEnabled( false );
		mProjectSplitter->setSplitPartition( StyleSheetLength( mConfig.window.panelPartition ) );

		if ( !mConfig.ui.showSidePanel )
			showSidePanel( mConfig.ui.showSidePanel );

		mEditorSplitter = UICodeEditorSplitter::New(
			this, mUISceneNode,
			SyntaxColorScheme::loadFromFile( mResPath + "assets/colorschemes/colorschemes.conf" ),
			mInitColorScheme );
		mEditorSplitter->setHideTabBarOnSingleTab( mConfig.editor.hideTabBarOnSingleTab );

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
		mFileWatcher = new efsw::FileWatcher();
		mFileSystemListener = new FileSystemListener( mEditorSplitter );
		mFileWatcher->watch();
#endif

		initSearchBar();

		initGlobalSearchBar();

		initLocateBar();

		createSettingsMenu();

		mEditorSplitter->createEditorWithTabWidget( mBaseLayout );

		mConsole = eeNew( Console, ( mFontMono, true, true, 1024 * 1000, 0, mWindow ) );

		initProjectTreeView( file );

		mWindow->runMainLoop( &appLoop );
	}
}

EE_MAIN_FUNC int main( int argc, char* argv[] ) {
#ifndef EE_DEBUG
	Log::create( LogLevel::Info, false, true );
#else
	Log::create( LogLevel::Debug, true, true );
#endif
	args::ArgumentParser parser( "ecode" );
	args::HelpFlag help( parser, "help", "Display this help menu", { 'h', "help" } );
	args::Positional<std::string> file( parser, "file", "The file path" );
	args::ValueFlag<Float> pixelDenstiyConf( parser, "pixel-density",
											 "Set default application pixel density",
											 { 'd', "pixel-density" } );

	try {
		parser.ParseCLI( argc, argv );
	} catch ( const args::Help& ) {
		std::cout << parser;
		return EXIT_SUCCESS;
	} catch ( const args::ParseError& e ) {
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return EXIT_FAILURE;
	} catch ( args::ValidationError& e ) {
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return EXIT_FAILURE;
	}

	appInstance = eeNew( App, () );
	appInstance->init( file.Get(), pixelDenstiyConf ? pixelDenstiyConf.Get() : 0.f );
	eeSAFE_DELETE( appInstance );

	Engine::destroySingleton();
	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
