#ifndef AUTOCOMPLETEMODULE_HPP
#define AUTOCOMPLETEMODULE_HPP

#include <eepp/config.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <set>
using namespace EE;
using namespace EE::System;
using namespace EE::UI;

class AutoCompleteModule : public UICodeEditorModule {
  public:
	typedef std::unordered_set<std::string> SymbolsList;

	AutoCompleteModule();

	AutoCompleteModule( std::shared_ptr<ThreadPool> pool );

	virtual ~AutoCompleteModule();

	void onRegister( UICodeEditor* );
	void onUnregister( UICodeEditor* );
	bool onKeyDown( UICodeEditor*, const KeyEvent& );
	bool onKeyUp( UICodeEditor*, const KeyEvent& );
	bool onTextInput( UICodeEditor*, const TextInputEvent& );
	void update( UICodeEditor* );
	void preDraw( UICodeEditor*, const Vector2f&, const Float&, const TextPosition& );
	void postDraw( UICodeEditor*, const Vector2f& startScroll, const Float& lineHeight,
				   const TextPosition& cursor );
	bool onMouseDown( UICodeEditor*, const Vector2i&, const Uint32& );
	bool onMouseClick( UICodeEditor*, const Vector2i&, const Uint32& );
	bool onMouseDoubleClick( UICodeEditor*, const Vector2i&, const Uint32& );
	bool onMouseMove( UICodeEditor*, const Vector2i&, const Uint32& );

	const Rectf& getBoxPadding() const;

	void setBoxPadding( const Rectf& boxPadding );

	const Uint32& getSuggestionsMaxVisible() const;

	void setSuggestionsMaxVisible( const Uint32& suggestionsMaxVisible );

	const Time& getUpdateFreq() const;

	void setUpdateFreq( const Time& updateFreq );

	const std::string& getSymbolPattern() const;

	void setSymbolPattern( const std::string& symbolPattern );

	bool isDirty() const;

	void setDirty( bool dirty );

  protected:
	std::string mSymbolPattern;
	Rectf mBoxPadding;
	std::shared_ptr<ThreadPool> mPool;
	Clock mClock;
	Mutex mLangSymbolsMutex;
	Mutex mSuggestionsMutex;
	Mutex mDocMutex;
	Time mUpdateFreq{Seconds( 5 )};
	std::unordered_map<UICodeEditor*, std::vector<Uint32>> mEditors;
	std::set<TextDocument*> mDocs;
	std::unordered_map<UICodeEditor*, TextDocument*> mEditorDocs;
	bool mDirty{false};
	bool mClosing{false};
	bool mReplacing{false};
	struct DocCache {
		Uint64 changeId{static_cast<Uint64>( -1 )};
		SymbolsList symbols;
	};
	std::unordered_map<TextDocument*, DocCache> mDocCache;
	std::unordered_map<std::string, SymbolsList> mLangCache;
	SymbolsList mLangDirty;

	int mSuggestionIndex{0};
	std::vector<std::string> mSuggestions;
	Uint32 mSuggestionsMaxVisible{8};
	UICodeEditor* mSuggestionsEditor{nullptr};

	Float mRowHeight{0};
	Rectf mBoxRect;

	void resetSuggestions( UICodeEditor* editor );

	void updateSuggestions( const std::string& symbol, UICodeEditor* editor );

	SymbolsList getDocumentSymbols( TextDocument* );

	void updateDocCache( TextDocument* doc );

	std::string getPartialSymbol( TextDocument* doc );

	void runUpdateSuggestions( const std::string& symbol, const SymbolsList& symbols,
							   UICodeEditor* editor );

	void updateLangCache( const std::string& langName );

	void pickSuggestion( UICodeEditor* editor );
};

#endif // AUTOCOMPLETEMODULE_HPP
