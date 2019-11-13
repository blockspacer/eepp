#ifndef EE_UI_UINODEDRAWABLE_HPP
#define EE_UI_UINODEDRAWABLE_HPP

#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/rectangledrawable.hpp>
#include <vector>
#include <map>

using namespace EE::Graphics;

namespace EE { namespace UI {

class EE_API UINodeDrawable : public Drawable {
	public:
		enum Repeat {
			RepeatXY,
			RepeatX,
			RepeatY,
			NoRepeat
		};

		static Repeat repeatFromText( const std::string& text );

		class EE_API LayerDrawable : public Drawable {
			public:
				static LayerDrawable * New( UINodeDrawable * container );

				LayerDrawable( UINodeDrawable * container );

				virtual ~LayerDrawable();

				bool isStateful() { return false; }

				virtual void draw();

				virtual void draw( const Vector2f& position );

				virtual void draw( const Vector2f& position, const Sizef& size );

				virtual Sizef getSize();

				virtual void setSize( const Sizef& size );

				Drawable* getDrawable() const;

				void setDrawable( Drawable* drawable, const bool& ownIt );

				const Vector2f& getOffset() const;

				void setPositionEq( const std::string& offset );

				const Repeat& getRepeat() const;

				void setRepeat( const Repeat& repeat );
			protected:
				UINodeDrawable * mContainer;
				Sizef mSize;
				Vector2f mOffset;
				std::string mPositionEq;
				bool mNeedsUpdate;
				bool mUpdatePosEq;
				bool mOwnsDrawable;
				Drawable * mDrawable;
				Uint32 mResourceChangeCbId;
				Repeat mRepeat;

				virtual void onPositionChange();

				void update();
		};

		static UINodeDrawable * New();

		UINodeDrawable();

		virtual ~UINodeDrawable();

		virtual Sizef getSize();

		virtual void setSize( const Sizef& size );

		virtual void draw();

		virtual void draw( const Vector2f& position );

		virtual void draw( const Vector2f& position, const Sizef& size );

		void draw( const Vector2f& position, const Sizef& size, const Uint32& alpha );

		virtual bool isStateful() { return false; }

		void clearDrawables();

		void setBorderRadius( const Uint32& corners );

		Uint32 getBorderRadius() const;

		LayerDrawable * getLayer( int index );

		void setDrawable( int index, Drawable * drawable, bool ownIt );

		void setDrawablePosition( int index, const std::string& positionEq );

		void setDrawableRepeat( int index, const std::string& repeatRule );

		void setBackgroundColor( const Color& color );

		Color getBackgroundColor() const;

		bool getClipEnabled() const;

		void setClipEnabled(bool clipEnabled);
	protected:
		RectangleDrawable mBackgroundColor;
		std::map<int,LayerDrawable*> mGroup;
		std::map<int,std::string> mPosEq;
		Sizef mSize;
		bool mNeedsUpdate;
		bool mClipEnabled;

		virtual void onPositionChange();

		virtual void onSizeChange();

		void update();
};

}}

#endif
