#ifndef EE_UICUISPRITE_HPP
#define EE_UICUISPRITE_HPP

#include "cuicontrolanim.hpp"

namespace EE { namespace UI {

class EE_API cUISprite : public cUIControlAnim {
	public:
		class CreateParams : public cUIControlAnim::CreateParams {
			public:
				inline CreateParams() : cUIControlAnim::CreateParams() {
					Sprite			= NULL;
					SpriteRender	= RN_NORMAL;
				}

				inline ~CreateParams() {}

				cSprite * 			Sprite;
				EE_RENDERTYPE		SpriteRender;
		};

		cUISprite( const cUISprite::CreateParams& Params );

		~cUISprite();

		virtual void Draw();

		virtual void Alpha( const eeFloat& alpha );

		cSprite * Sprite() const;

		void Sprite( cSprite * sprite );

		eeColorA Color() const;

		void Color( const eeColorA& color );

		const EE_RENDERTYPE& RenderType() const;

		void RenderType( const EE_RENDERTYPE& render );
	protected:
		cSprite * 			mSprite;
		EE_RENDERTYPE 		mRender;

		void UpdateSize();
};

}}

#endif
