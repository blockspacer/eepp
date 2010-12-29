#ifndef EE_UICUITOOLTIP_HPP
#define EE_UICUITOOLTIP_HPP

#include "cuicontrolanim.hpp"

namespace EE { namespace UI {

class EE_API cUITooltip : public cUIControlAnim {
	public:
		class CreateParams : public cUIControlAnim::CreateParams {
			public:
				inline CreateParams() :
					cUIControlAnim::CreateParams(),
					Font( NULL ),
					FontColor( 0, 0, 0, 255 ),
					FontShadowColor( 255, 255, 255, 150 )
				{
					cUITheme * Theme = cUIThemeManager::instance()->DefaultTheme();

					if ( NULL != Theme ) {
						Font			= Theme->Font();
						FontColor		= Theme->FontColor();
						FontShadowColor	= Theme->FontShadowColor();
					}

					if ( NULL == Font )
						Font = cUIThemeManager::instance()->DefaultFont();
				}

				inline ~CreateParams() {}

				cFont * 	Font;
				eeColorA 	FontColor;
				eeColorA 	FontShadowColor;
		};

		cUITooltip( cUITooltip::CreateParams& Params );

		~cUITooltip();

		virtual void SetTheme( cUITheme * Theme );

		void Show();

		void Hide();

		virtual void Draw();

		virtual void Alpha( const eeFloat& alpha );

		cFont * Font() const;

		void Font( cFont * font );

		virtual const std::wstring& Text();

		virtual void Text( const std::wstring& text );

		virtual void Text( const std::string& text );

		const eeColorA& Color() const;

		void Color( const eeColorA& color );

		const eeColorA& ShadowColor() const;

		void ShadowColor( const eeColorA& color );

		virtual void OnTextChanged();

		virtual void OnFontChanged();

		virtual void Padding( const eeRecti& padding );

		const eeRecti& Padding() const;

		cTextCache * GetTextCache();

		eeFloat GetTextWidth();

		eeFloat GetTextHeight();

		const eeInt& GetNumLines() const;

		const eeVector2f& AlignOffset() const;

		void TooltipTime( const eeFloat& Time );

		void TooltipTimeAdd( const eeFloat& Time );

		const eeFloat& TooltipTime() const;
	protected:
		cTextCache *	mTextCache;
		eeColorA 		mFontColor;
		eeColorA 		mFontShadowColor;
		eeVector2f 		mAlignOffset;
		eeRecti			mPadding;
		eeFloat			mTooltipTime;

		virtual void OnSizeChange();

		virtual void AutoSize();

		virtual void AutoAlign();

		virtual void AutoPadding();
};

}}

#endif