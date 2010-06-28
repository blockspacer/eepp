#ifndef EE_WINDOWCINPUTTEXTBUFFER_H
#define EE_WINDOWCINPUTTEXTBUFFER_H

#include "base.hpp"
#include "cinput.hpp"

namespace EE { namespace Window {

class EE_API cInputTextBuffer {
	public:
		typedef boost::function0<void> EnterCallback;
		
		cInputTextBuffer( const bool& Active, const bool& SupportNewLine, const bool& SupportFreeEditing, const Uint32& MaxLenght = 0xFFFFFFFF );
		cInputTextBuffer();
		~cInputTextBuffer();
		
		/** @return The current buffer */
		std::wstring Buffer() const { return mText; };

		/** Set a new current buffer */
		void Buffer( const std::wstring& str );
		
		/** @return If input buffer is active */
		bool Active() const { return mActive; }

		/** Set the state of the input buffer */
		void Active( const bool& Active ) { mActive = Active; }
		
		/** @return If new line is supported */
		bool SupportNewLine() const { return mSupportNewLine; }

		/** Support new line consist of allowing to add a new line when key return is pressed. */
		void SupportNewLine( const bool& SupportNewLine ) { mSupportNewLine = SupportNewLine; }
		
		/** @return If Free Editing is enabled */
		bool SupportFreeEditing() const { return mPromtPosSupport; }

		/** Free editing consist on the capability of moving the cursor position over the buffer, to write over the buffer, and not only after the last character. */
		void SupportFreeEditing( const bool& SupportNewLine ) { mPromtPosSupport = SupportNewLine; }
		
		/** Start the input buffer */
		void Start();

		/** Clear the buffer */
		void Clear();

		/** Internal callback, don't call it */
		void Update( EE_Event* Event );
		
		/** A callback for the key return */
		void SetReturnCallback( EnterCallback EC );
		
		/** @return If something changed since last update */
		bool ChangedSinceLastUpdate();
		
		/** @return The Cursor Position (where is the cursor editing) */
		eeInt CurPos() const;

		/** Set the cursor position */
		void CurPos( const Uint32& pos );
		
		/** This function it's for helping the cFont class to locate the cursor position for the correct rendering of it.
		* @param LastNewLinePos This will return the position of the closest "\n" to the current Cursor Pos
		* @return One which line it's the cursor
		*/
		Uint32 GetCurPosLinePos( Uint32& LastNewLinePos );
		
		/** Push the char you want to ignore */
		void PushIgnoredChar( const Uint32& ch );
		
		/** Set the new max lenght */
		void MaxLenght( const Uint32& Max );
		
		/** @return The Max Lenght */
		const Uint32& MaxLenght() const;
	protected:
		void SetAutoPromp( const bool& set = true );
		
		std::wstring mText;
		bool mActive;
		bool mSupportNewLine;
		bool mChangeSinceLastUpdate;
		Uint32 mCallback;
		
		bool mPromtPosSupport;
		eeInt mPromptPos;
		bool mPromptAutoPos;
		
		bool mEnter;
		EnterCallback mEnterCall;
		
		Uint32 mMaxLenght;

		std::vector<Uint32> mIgnoredChars;
		
		bool CanAdd();
};

}}

#endif