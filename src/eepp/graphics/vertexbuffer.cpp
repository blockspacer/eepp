#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/vertexbuffer.hpp>
#include <eepp/graphics/vertexbuffermanager.hpp>
#include <eepp/graphics/vertexbufferogl.hpp>
#include <eepp/graphics/vertexbuffervbo.hpp>
using namespace EE::Graphics::Private;

namespace EE { namespace Graphics {

VertexBuffer* VertexBuffer::New( const Uint32& VertexFlags, PrimitiveType DrawType,
								 const Int32& ReserveVertexSize, const Int32& ReserveIndexSize,
								 VertexBufferUsageType UsageType ) {
	if ( GLi->isExtension( EEGL_ARB_vertex_buffer_object ) )
		return eeNew( VertexBufferVBO,
					  ( VertexFlags, DrawType, ReserveVertexSize, ReserveIndexSize, UsageType ) );

	return eeNew( VertexBufferOGL,
				  ( VertexFlags, DrawType, ReserveVertexSize, ReserveIndexSize, UsageType ) );
}

VertexBuffer* VertexBuffer::NewVertexArray( const Uint32& VertexFlags, PrimitiveType DrawType,
											const Int32& ReserveVertexSize,
											const Int32& ReserveIndexSize,
											VertexBufferUsageType UsageType ) {
	return eeNew( VertexBufferOGL,
				  ( VertexFlags, DrawType, ReserveVertexSize, ReserveIndexSize, UsageType ) );
}

VertexBuffer::VertexBuffer( const Uint32& VertexFlags, PrimitiveType DrawType,
							const Int32& ReserveVertexSize, const Int32& ReserveIndexSize,
							VertexBufferUsageType UsageType ) :
	mVertexFlags( VertexFlags ), mDrawType( DrawType ), mUsageType( UsageType ), mElemDraw( -1 ) {
	if ( ReserveVertexSize > 0 ) {
		for ( Int32 i = 0; i < VERTEX_FLAGS_COUNT; i++ ) {
			if ( VERTEX_FLAG_QUERY( mVertexFlags, i ) ) {
				if ( i != VERTEX_FLAG_COLOR )
					mVertexArray[i].reserve( ReserveVertexSize * VertexElementCount[i] );
				else
					mColorArray.reserve( ReserveVertexSize * VertexElementCount[i] );
			}
		}
	}

	if ( ReserveIndexSize > 0 ) {
		mIndexArray.reserve( ReserveIndexSize );
	}

	VertexBufferManager::instance()->add( this );
}

VertexBuffer::~VertexBuffer() {
	VertexBufferManager::instance()->remove( this );
}

void VertexBuffer::addVertex( const Uint32& Type, const Vector2f& Vertex ) {
	if ( Type < VERTEX_FLAGS_COUNT_ARR ) {
		mVertexArray[Type].push_back( Vertex.x );
		mVertexArray[Type].push_back( Vertex.y );
	}
}

void VertexBuffer::addVertex( const Vector2f& Vertex ) {
	addVertex( VERTEX_FLAG_POSITION, Vertex );
}

void VertexBuffer::addTextureCoord( const Vector2f& VertexCoord, const Uint32& TextureLevel ) {
	addVertex( VERTEX_FLAG_TEXTURE0 + TextureLevel, VertexCoord );
}

void VertexBuffer::addColor( const Color& Color ) {
	mColorArray.push_back( Color.r );
	mColorArray.push_back( Color.g );
	mColorArray.push_back( Color.b );
	mColorArray.push_back( Color.a );
}

void VertexBuffer::addIndex( const Uint32& Index ) {
	mIndexArray.push_back( Index );

	VERTEX_FLAG_SET( mVertexFlags, VERTEX_FLAG_USE_INDICES );
}

void VertexBuffer::resizeArray( const Uint32& Type, const Uint32& Size ) {
	if ( Type != VERTEX_FLAG_COLOR )
		mVertexArray[Type].resize( Size );
	else
		mColorArray.resize( Size );
}

void VertexBuffer::resizeIndices( const Uint32& Size ) {
	mIndexArray.resize( Size );
}

Float* VertexBuffer::getArray( const Uint32& Type ) {
	if ( Type < VERTEX_FLAGS_COUNT_ARR && mVertexArray[Type].size() )
		return &mVertexArray[Type - 1][0];

	return NULL;
}

Uint8* VertexBuffer::getColorArray() {
	if ( mColorArray.size() )
		return &mColorArray[0];

	return NULL;
}

Uint32* VertexBuffer::getIndices() {
	if ( mIndexArray.size() )
		return &mIndexArray[0];

	return NULL;
}

Uint32 VertexBuffer::getVertexCount() {
	return (Uint32)mVertexArray[VERTEX_FLAG_POSITION].size() /
		   VertexElementCount[VERTEX_FLAG_POSITION];
}

Uint32 VertexBuffer::getIndexCount() {
	return (Uint32)mIndexArray.size();
}

Vector2f VertexBuffer::getVector2( const Uint32& Type, const Uint32& Index ) {
	eeASSERT( Type < VERTEX_FLAGS_COUNT_ARR && !VERTEX_FLAG_QUERY( mVertexFlags, Type ) )

		Int32 pos = Index * VertexElementCount[Type];

	return Vector2f( mVertexArray[Type][pos], mVertexArray[Type][pos + 1] );
}

Color VertexBuffer::getColor( const Uint32& Index ) {
	eeASSERT( !VERTEX_FLAG_QUERY( mVertexFlags, VERTEX_FLAG_COLOR ) );

	Int32 pos = Index * VertexElementCount[VERTEX_FLAG_COLOR];

	return Color( mColorArray[pos], mColorArray[pos + 1], mColorArray[pos + 2],
				  mColorArray[pos + 3] );
}

Uint32 VertexBuffer::getIndex( const Uint32& Index ) {
	eeASSERT( Index < mIndexArray.size() );
	return mIndexArray[Index];
}

void VertexBuffer::setElementNum( Int32 Num ) {
	mElemDraw = Num;
}

const Int32& VertexBuffer::getElementNum() const {
	return mElemDraw;
}

void VertexBuffer::clear() {
	for ( int i = 0; i < VERTEX_FLAGS_COUNT; i++ )
		mVertexArray[i].clear();

	mColorArray.clear();
	mIndexArray.clear();
}

}} // namespace EE::Graphics
