﻿#ifndef EE_UI_UICODEEDIT_HPP
#define EE_UI_UICODEEDIT_HPP

#include <eepp/ui/doc/syntaxcolorscheme.hpp>
#include <eepp/ui/doc/syntaxhighlighter.hpp>
#include <eepp/ui/doc/textdocument.hpp>
#include <eepp/ui/keyboardshortcut.hpp>
#include <eepp/ui/uifontstyleconfig.hpp>
#include <eepp/ui/uiwidget.hpp>

using namespace EE::Graphics;
using namespace EE::UI::Doc;

namespace EE { namespace Graphics {
class Font;
}} // namespace EE::Graphics

namespace EE { namespace UI {

class UICodeEditor;
class UIScrollBar;

class UICodeEditorModule {
  public:
	virtual void onRegister( UICodeEditor* ) = 0;
	virtual void onUnregister( UICodeEditor* ) = 0;
	virtual bool onKeyDown( UICodeEditor*, const KeyEvent& ) { return false; }
	virtual bool onKeyUp( UICodeEditor*, const KeyEvent& ) { return false; }
	virtual bool onTextInput( UICodeEditor*, const TextInputEvent& ) { return false; }
	virtual void update( UICodeEditor* ) {}
	virtual void preDraw( UICodeEditor*, const Vector2f& /*startScroll*/,
						  const Float& /*lineHeight*/, const TextPosition& /*cursor*/ ) {}
	virtual void postDraw( UICodeEditor*, const Vector2f& /*startScroll*/,
						   const Float& /*lineHeight*/, const TextPosition& /*cursor*/ ) {}
	virtual void onFocus( UICodeEditor* ) {}
	virtual void onFocusLoss( UICodeEditor* ) {}
	virtual bool onMouseDown( UICodeEditor*, const Vector2i&, const Uint32& ) { return false; }
	virtual bool onMouseMove( UICodeEditor*, const Vector2i&, const Uint32& ) { return false; }
	virtual bool onMouseUp( UICodeEditor*, const Vector2i&, const Uint32& ) { return false; }
	virtual bool onMouseClick( UICodeEditor*, const Vector2i&, const Uint32& ) { return false; }
	virtual bool onMouseDoubleClick( UICodeEditor*, const Vector2i&, const Uint32& ) {
		return false;
	}
	virtual bool onMouseOver( UICodeEditor*, const Vector2i&, const Uint32& ) { return false; }
	virtual bool onMouseLeave( UICodeEditor*, const Vector2i&, const Uint32& ) { return false; }

	virtual void drawBeforeLineText( UICodeEditor*, const Int64&, Vector2f, const Float&,
									 const Float& ){};
	virtual void drawAfterLineText( UICodeEditor*, const Int64&, Vector2f, const Float&,
									const Float& ){};
};

class EE_API DocEvent : public Event {
  public:
	DocEvent( Node* node, TextDocument* doc, const Uint32& eventType ) :
		Event( node, eventType ), doc( doc ) {}
	TextDocument* getDoc() const { return doc; }

  protected:
	TextDocument* doc;
};

class EE_API DocSyntaxDefEvent : public DocEvent {
  public:
	DocSyntaxDefEvent( Node* node, TextDocument* doc, const Uint32& eventType,
					   const std::string& oldLang, const std::string& newLang ) :
		DocEvent( node, doc, eventType ), oldLang( oldLang ), newLang( newLang ) {}
	const std::string& getOldLang() const { return oldLang; }
	const std::string& getNewLang() const { return newLang; }

  protected:
	TextDocument* doc;
	std::string oldLang;
	std::string newLang;
};

class EE_API UICodeEditor : public UIWidget, public TextDocument::Client {
  public:
	static UICodeEditor* New();

	static UICodeEditor* NewOpt( const bool& autoRegisterBaseCommands,
								 const bool& autoRegisterBaseKeybindings );

	static const std::map<KeyBindings::Shortcut, std::string> getDefaultKeybindings();

	UICodeEditor( const bool& autoRegisterBaseCommands = true,
				  const bool& autoRegisterBaseKeybindings = true );

	virtual ~UICodeEditor();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

	virtual void draw();

	virtual void scheduledUpdate( const Time& time );

	void reset();

	bool loadFromFile( const std::string& path );

	bool save();

	bool save( const std::string& path );

	bool save( IOStreamFile& stream );

	Font* getFont() const;

	const UIFontStyleConfig& getFontStyleConfig() const;

	UICodeEditor* setFont( Font* font );

	UICodeEditor* setFontSize( const Float& dpSize );

	const Float& getFontSize() const;

	UICodeEditor* setFontColor( const Color& color );

	const Color& getFontColor() const;

	UICodeEditor* setFontSelectedColor( const Color& color );

	const Color& getFontSelectedColor();

	UICodeEditor* setFontSelectionBackColor( const Color& color );

	const Color& getFontSelectionBackColor() const;

	UICodeEditor* setFontShadowColor( const Color& color );

	const Color& getFontShadowColor() const;

	UICodeEditor* setFontStyle( const Uint32& fontStyle );

	const Uint32& getTabWidth() const;

	UICodeEditor* setTabWidth( const Uint32& tabWidth );

	const Uint32& getFontStyle() const;

	const Float& getOutlineThickness() const;

	UICodeEditor* setOutlineThickness( const Float& outlineThickness );

	const Color& getOutlineColor() const;

	UICodeEditor* setOutlineColor( const Color& outlineColor );

	const Float& getMouseWheelScroll() const;

	void setMouseWheelScroll( const Float& mouseWheelScroll );

	void setLineNumberPaddingLeft( const Float& dpLeft );

	void setLineNumberPaddingRight( const Float& dpRight );

	void setLineNumberPadding( const Float& dpPaddingLeft, const Float& dpPaddingRight );

	const Float& getLineNumberPaddingLeft() const;

	const Float& getLineNumberPaddingRight() const;

	size_t getLineNumberDigits() const;

	Float getLineNumberWidth() const;

	const bool& getShowLineNumber() const;

	void setShowLineNumber( const bool& showLineNumber );

	const Color& getLineNumberBackgroundColor() const;

	void setLineNumberBackgroundColor( const Color& lineNumberBackgroundColor );

	const Color& getCurrentLineBackgroundColor() const;

	void setCurrentLineBackgroundColor( const Color& currentLineBackgroundColor );

	const Color& getCaretColor() const;

	void setCaretColor( const Color& caretColor );

	const Color& getWhitespaceColor() const;

	void setWhitespaceColor( const Color& color );

	const SyntaxColorScheme& getColorScheme() const;

	void setColorScheme( const SyntaxColorScheme& colorScheme );

	/** If the document is managed by more than one client you need to NOT auto register base
	 * commands and implement your own logic for those commands, since are dependant of the client
	 * state.
	 * @see registerCommands */
	std::shared_ptr<Doc::TextDocument> getDocumentRef() const;

	const Doc::TextDocument& getDocument() const;

	Doc::TextDocument& getDocument();

	void setDocument( std::shared_ptr<TextDocument> doc );

	bool isDirty() const;

	const bool& isLocked() const;

	void setLocked( bool locked );

	const Color& getLineNumberFontColor() const;

	void setLineNumberFontColor( const Color& lineNumberFontColor );

	const Color& getLineNumberActiveFontColor() const;

	void setLineNumberActiveFontColor( const Color& lineNumberActiveFontColor );

	bool isTextSelectionEnabled() const;

	void setTextSelection( const bool& active );

	KeyBindings& getKeyBindings();

	void setKeyBindings( const KeyBindings& keyBindings );

	void addKeyBindingString( const std::string& shortcut, const std::string& command,
							  const bool& allowLocked = false );

	void addKeyBinding( const KeyBindings::Shortcut& shortcut, const std::string& command,
						const bool& allowLocked = false );

	void replaceKeyBindingString( const std::string& shortcut, const std::string& command,
								  const bool& allowLocked = false );

	void replaceKeyBinding( const KeyBindings::Shortcut& shortcut, const std::string& command,
							const bool& allowLocked = false );

	void addKeyBindsString( const std::map<std::string, std::string>& binds,
							const bool& allowLocked = false );

	void addKeyBinds( const std::map<KeyBindings::Shortcut, std::string>& binds,
					  const bool& allowLocked = false );

	const bool& getHighlightCurrentLine() const;

	void setHighlightCurrentLine( const bool& highlightCurrentLine );

	const Uint32& getLineBreakingColumn() const;

	/** Set to 0 to hide. */
	void setLineBreakingColumn( const Uint32& lineBreakingColumn );

	void addUnlockedCommand( const std::string& command );

	void addUnlockedCommands( const std::vector<std::string>& commands );

	bool isUnlockedCommand( const std::string& command );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 );

	const bool& getHighlightMatchingBracket() const;

	void setHighlightMatchingBracket( const bool& highlightMatchingBracket );

	const Color& getMatchingBracketColor() const;

	void setMatchingBracketColor( const Color& matchingBracketColor );

	const bool& getHighlightSelectionMatch() const;

	void setHighlightSelectionMatch( const bool& highlightSelection );

	const Color& getSelectionMatchColor() const;

	void setSelectionMatchColor( const Color& highlightSelectionMatchColor );

	const bool& getEnableColorPickerOnSelection() const;

	void setEnableColorPickerOnSelection( const bool& enableColorPickerOnSelection );

	void setSyntaxDefinition( const SyntaxDefinition& definition );

	const SyntaxDefinition& getSyntaxDefinition() const;

	const bool& getHorizontalScrollBarEnabled() const;

	void setHorizontalScrollBarEnabled( const bool& horizontalScrollBarEnabled );

	const Time& getFindLongestLineWidthUpdateFrequency() const;

	void setFindLongestLineWidthUpdateFrequency( const Time& findLongestLineWidthUpdateFrequency );

	/** Doc commands executed in this editor. */
	TextPosition moveToLineOffset( const TextPosition& position, int offset );

	void moveToPreviousLine();

	void moveToNextLine();

	void selectToPreviousLine();

	void selectToNextLine();

	void registerKeybindings();

	void registerCommands();

	void moveScrollUp();

	void moveScrollDown();

	void indent();

	void unindent();

	void copy();

	void cut();

	void paste();

	void fontSizeGrow();

	void fontSizeShrink();

	void fontSizeReset();
	/** Doc commands executed in this editor. */

	const bool& getShowWhitespaces() const;

	void setShowWhitespaces( const bool& showWhitespaces );

	const String& getHighlightWord() const;

	void setHighlightWord( const String& highlightWord );

	const TextRange& getHighlightTextRange() const;

	void setHighlightTextRange( const TextRange& highlightSelection );

	void registerModule( UICodeEditorModule* module );

	void unregisterModule( UICodeEditorModule* module );

	virtual Int64 getColFromXOffset( Int64 line, const Float& x ) const;

	virtual Float getColXOffset( TextPosition position );

	virtual Float getXOffsetCol( const TextPosition& position );

	virtual Float getLineWidth( const Int64& lineIndex );

	Float getTextWidth( const String& text ) const;

	Float getLineHeight() const;

	Float getCharacterSize() const;

	Float getGlyphWidth() const;

	const bool& getColorPreview() const;

	void setColorPreview( bool colorPreview );

	void goToLine( const TextPosition& position, bool centered = true );

	bool getAutoCloseBrackets() const;

	void setAutoCloseBrackets( bool autoCloseBracket );

  protected:
	struct LastXOffset {
		TextPosition position;
		Float offset;
	};
	Font* mFont;
	UIFontStyleConfig mFontStyleConfig;
	std::shared_ptr<Doc::TextDocument> mDoc;
	Vector2f mScrollPos;
	Clock mBlinkTimer;
	bool mDirtyEditor;
	bool mCursorVisible;
	bool mMouseDown;
	bool mShowLineNumber;
	bool mShowWhitespaces;
	bool mLocked;
	bool mHighlightCurrentLine;
	bool mHighlightMatchingBracket;
	bool mHighlightSelectionMatch;
	bool mEnableColorPickerOnSelection;
	bool mHorizontalScrollBarEnabled;
	bool mLongestLineWidthDirty;
	bool mColorPreview;
	Uint32 mTabWidth;
	Vector2f mScroll;
	Float mMouseWheelScroll;
	Float mFontSize;
	Float mLineNumberPaddingLeft;
	Float mLineNumberPaddingRight;
	Color mLineNumberFontColor;
	Color mLineNumberActiveFontColor;
	Color mLineNumberBackgroundColor;
	Color mCurrentLineBackgroundColor;
	Color mCaretColor;
	Color mWhitespaceColor;
	Color mLineBreakColumnColor;
	Color mMatchingBracketColor;
	Color mSelectionMatchColor;
	SyntaxColorScheme mColorScheme;
	SyntaxHighlighter mHighlighter;
	UIScrollBar* mVScrollBar;
	UIScrollBar* mHScrollBar;
	LastXOffset mLastXOffset{ { 0, 0 }, 0.f };
	KeyBindings mKeyBindings;
	std::unordered_set<std::string> mUnlockedCmd;
	Clock mLastDoubleClick;
	Uint32 mLineBreakingColumn{ 100 };
	TextRange mMatchingBrackets;
	Float mLongestLineWidth{ 0 };
	Time mFindLongestLineWidthUpdateFrequency;
	Clock mLongestLineWidthLastUpdate;
	String mHighlightWord;
	TextRange mHighlightTextRange;
	Color mPreviewColor;
	TextRange mPreviewColorRange;
	std::vector<UICodeEditorModule*> mModules;

	UICodeEditor( const std::string& elementTag, const bool& autoRegisterBaseCommands = true,
				  const bool& autoRegisterBaseKeybindings = true );

	void checkMatchingBrackets();

	void updateColorScheme();

	void updateLongestLineWidth();

	void invalidateEditor();

	void invalidateLongestLineWidth();

	virtual void findLongestLine();

	virtual Uint32 onFocus();

	virtual Uint32 onFocusLoss();

	virtual Uint32 onTextInput( const TextInputEvent& event );

	virtual Uint32 onKeyDown( const KeyEvent& event );

	virtual Uint32 onKeyUp( const KeyEvent& event );

	virtual Uint32 onMouseDown( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseMove( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseUp( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseClick( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseDoubleClick( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseOver( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseLeave( const Vector2i& position, const Uint32& flags );

	virtual void onSizeChange();

	virtual void onPaddingChange();

	virtual void onCursorPosChange();

	void updateEditor();

	virtual void onDocumentTextChanged();

	virtual void onDocumentCursorChange( const TextPosition& );

	virtual void onDocumentSelectionChange( const TextRange& );

	virtual void onDocumentLineCountChange( const size_t& lastCount, const size_t& newCount );

	virtual void onDocumentLineChanged( const Int64& lineIndex );

	virtual void onDocumentUndoRedo( const TextDocument::UndoRedo& );

	virtual void onDocumentSaved( TextDocument* );

	virtual void onDocumentClosed( TextDocument* doc );

	virtual void onDocumentDirtyOnFileSystem( TextDocument* doc );

	std::pair<int, int> getVisibleLineRange();

	int getVisibleLinesCount();

	void scrollToMakeVisible( const TextPosition& position, bool centered = false );

	void setScrollX( const Float& val, bool emmitEvent = true );

	void setScrollY( const Float& val, bool emmitEvent = true );

	void resetCursor();

	TextPosition resolveScreenPosition( const Vector2f& position ) const;

	Vector2f getViewPortLineCount() const;

	Sizef getMaxScroll() const;

	void updateScrollBar();

	void checkColorPickerAction();

	virtual void drawCursor( const Vector2f& startScroll, const Float& lineHeight,
							 const TextPosition& cursor );

	virtual void drawMatchingBrackets( const Vector2f& startScroll, const Float& lineHeight );

	virtual void drawSelectionMatch( const std::pair<int, int>& lineRange,
									 const Vector2f& startScroll, const Float& lineHeight );

	virtual void drawWordMatch( const String& text, const std::pair<int, int>& lineRange,
								const Vector2f& startScroll, const Float& lineHeight );

	virtual void drawLineText( const Int64& index, Vector2f position, const Float& fontSize,
							   const Float& lineHeight );

	virtual void drawWhitespaces( const std::pair<int, int>& lineRange, const Vector2f& startScroll,
								  const Float& lineHeight );

	virtual void drawTextRange( const TextRange& range, const std::pair<int, int>& lineRange,
								const Vector2f& startScroll, const Float& lineHeight,
								const Color& backgrundColor );

	virtual void drawLineNumbers( const std::pair<int, int>& lineRange, const Vector2f& startScroll,
								  const Vector2f& screenStart, const Float& lineHeight,
								  const Float& lineNumberWidth, const int& lineNumberDigits,
								  const Float& fontSize );

	virtual void drawColorPreview( const Vector2f& startScroll, const Float& lineHeight );

	virtual void onFontChanged();

	virtual void onFontStyleChanged();

	virtual void onDocumentChanged();

	virtual Uint32 onMessage( const NodeMessage* msg );

	void checkMouseOverColor( const Vector2i& position );

	void resetPreviewColor();

	void disableEditorFeatures();

	Float getViewportWidth( const bool& forceVScroll = false ) const;
};

}} // namespace EE::UI

#endif // EE_UI_UICODEEDIT_HPP
