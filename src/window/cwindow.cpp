#include "cwindow.hpp"
#include "cclipboard.hpp"
#include "cinput.hpp"
#include "ccursormanager.hpp"
#include "../graphics/ctexturefactory.hpp"
#include "platform/null/cnullimpl.hpp"

namespace EE { namespace Window {

cWindow::cWindow( WindowSettings Settings, ContextSettings Context, cClipboard * Clipboard, cInput * Input, cCursorManager * CursorManager ) :
	mClipboard( Clipboard ),
	mInput( Input ),
	mCursorManager( CursorManager ),
	mPlatform( NULL ),
	mNumCallBacks( 0 )
{
	mWindow.WindowConfig	= Settings;
	mWindow.ContextConfig	= Context;
}

cWindow::~cWindow() {
	eeSAFE_DELETE( mClipboard );
	eeSAFE_DELETE( mInput );
	eeSAFE_DELETE( mCursorManager );
	eeSAFE_DELETE( mPlatform );
}

eeSize cWindow::Size() {
	return eeSize( mWindow.WindowConfig.Width, mWindow.WindowConfig.Height );
}

const Uint32& cWindow::GetWidth() const {
	return mWindow.WindowConfig.Width;
}

const Uint32& cWindow::GetHeight() const {
	return mWindow.WindowConfig.Height;
}

const eeSize& cWindow::GetDesktopResolution() const {
	return mWindow.DesktopResolution;
}

void cWindow::Size( Uint32 Width, Uint32 Height ) {
	Size( Width, Height, Windowed() );
}

bool cWindow::Windowed() const {
	return 0 != !( mWindow.WindowConfig.Style & WindowStyle::Fullscreen );
}

bool cWindow::Resizeable() const {
	return 0 != ( mWindow.WindowConfig.Style & WindowStyle::Resize );
}

void cWindow::SetViewport( const Int32& x, const Int32& y, const Uint32& Width, const Uint32& Height, const bool& UpdateProjectionMatrix ) {
	GLi->Viewport( x, GetHeight() - Height - y, Width, Height );
	
	if ( UpdateProjectionMatrix ) {
		GLi->MatrixMode( GL_PROJECTION );
		GLi->LoadIdentity();

		GLi->Ortho( 0.0f, Width, Height, 0.0f, -1000.0f, 1000.0f );

		GLi->MatrixMode( GL_MODELVIEW );
		GLi->LoadIdentity();
	}
}

void cWindow::SetView( const cView& View ) {
	mCurrentView = &View;

	eeRecti RView = mCurrentView->GetView();
	SetViewport( RView.Left, RView.Top, RView.Right, RView.Bottom );
}

const cView& cWindow::GetDefaultView() const {
	return mDefaultView;
}

const cView& cWindow::GetView() const {
    return *mCurrentView;
}

void cWindow::CreateView() {
	mDefaultView.SetView( 0, 0, mWindow.WindowConfig.Width, mWindow.WindowConfig.Height );
	mCurrentView = &mDefaultView;
}

void cWindow::Setup2D( const bool& KeepView ) {
	BackColor( mWindow.BackgroundColor );

	GLi->LineSmooth();

	GLi->Enable	( GL_TEXTURE_2D ); 						// Enable Textures
	GLi->Disable( GL_DEPTH_TEST );
	GLi->Disable( GL_LIGHTING );

	if ( !KeepView )
		SetView( mDefaultView );
	
	cTextureFactory::instance()->SetPreBlendFunc( ALPHA_NORMAL, true );

	if ( GLv_3 != GLi->Version() ) {
		#ifndef EE_GLES
		GLi->TexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		GLi->TexEnvi( GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE );
		#endif
	}

	GLi->EnableClientState( GL_VERTEX_ARRAY );
	GLi->EnableClientState( GL_TEXTURE_COORD_ARRAY );
	GLi->EnableClientState( GL_COLOR_ARRAY );
}

const WindowInfo * cWindow::GetWindowInfo() const {
	return &mWindow;
}

void cWindow::BackColor( const eeColor& Color ) {
	mWindow.BackgroundColor = Color;
	GLi->ClearColor( static_cast<eeFloat>( mWindow.BackgroundColor.R() ) / 255.0f, static_cast<eeFloat>( mWindow.BackgroundColor.G() ) / 255.0f, static_cast<eeFloat>( mWindow.BackgroundColor.B() ) / 255.0f, 255.0f );
}

const eeColor& cWindow::BackColor() const {
	return mWindow.BackgroundColor;
}

bool cWindow::TakeScreenshot( std::string filepath, const EE_SAVE_TYPE& Format ) {
	bool CreateNewFile = false;
	std::string File, Ext;

	if ( filepath.size() ) {
		File = filepath.substr( filepath.find_last_of("/\\") + 1 );
		Ext = File.substr( File.find_last_of(".") + 1 );

		if ( IsDirectory( filepath ) || !Ext.size() )
			CreateNewFile = true;
	} else {
		filepath = AppPath();
		CreateNewFile = true;
	}

	if ( CreateNewFile ) { // Search if file path is given, and if have and extension
		bool find = false;
		Int32 FileNum = 1;
		std::string TmpPath = filepath, Ext;

		if ( !IsDirectory( filepath ) )
			MakeDir( filepath );

		Ext = "." + SaveTypeToExtension( Format );

		while ( !find && FileNum < 10000 ) {
			TmpPath = StrFormated( "%s%05d%s", filepath.c_str(), FileNum, Ext.c_str() );

			FileNum++;

			if ( !FileExists( TmpPath ) )
				find = true;

			if ( FileNum == 10000 && find == false )
				return false;
		}

		return 0 != SOIL_save_screenshot(TmpPath.c_str(), Format, 0, 0, mWindow.WindowConfig.Width, mWindow.WindowConfig.Height );
	} else {
		std::string Direc = filepath.substr( 0, filepath.find_last_of("/\\") );
		if ( !IsDirectory( Direc ) )
			MakeDir( Direc );

		return 0 != SOIL_save_screenshot(filepath.c_str(), Format, 0, 0, mWindow.WindowConfig.Width, mWindow.WindowConfig.Height );
	}
}

bool cWindow::Running() const {
	return mWindow.Created;
}

bool cWindow::Created() const {
	return mWindow.Created;
}

void cWindow::Close() {
	mWindow.Created = false;
}

void cWindow::FrameRateLimit( const Uint32& FrameRateLimit ) {
	mFrameData.FPS.Limit = (eeFloat)FrameRateLimit;
}

Uint32 cWindow::FrameRateLimit() {
	return static_cast<Uint32>( mFrameData.FPS.Limit );
}

Uint32 cWindow::FPS() const {
	return mFrameData.FPS.Current;
}

eeFloat cWindow::Elapsed() const {
	return mFrameData.ElapsedTime;
}

void cWindow::GetElapsedTime() {
	mFrameData.ElapsedTime = static_cast<eeFloat> ( mFrameData.FrameElapsed.Elapsed() );
}

void cWindow::CalculateFps() {
	if ( eeGetTicks() - mFrameData.FPS.LastCheck >= 1000 ) {
		mFrameData.FPS.Current = mFrameData.FPS.Count;
		mFrameData.FPS.Count = 0;
		mFrameData.FPS.LastCheck =	eeGetTicks();
	}

	mFrameData.FPS.Count++;
}

void cWindow::LimitFps() {
	if ( mFrameData.FPS.Limit > 0 ) {
		mFrameData.FPS.Error = 0;
		eeFloat RemainT = 1000.f / mFrameData.FPS.Limit - ( mFrameData.ElapsedTime * 0.1f );

		if ( RemainT < 0 ) {
			mFrameData.FPS.Error = 0;
		} else {
			mFrameData.FPS.Error += 1000 % (Int32)mFrameData.FPS.Limit;

			if ( mFrameData.FPS.Error > (Int32)mFrameData.FPS.Limit ) {
				++RemainT;
				mFrameData.FPS.Error -= (Int32)mFrameData.FPS.Limit;
			}

			if ( RemainT > 0 ) {
				eeSleep( (Uint32) RemainT );
			}
		}
	}
}

void cWindow::ViewCheckUpdate() {
	if ( mCurrentView->NeedUpdate() ) {
		SetView( *mCurrentView );
	}
}

void cWindow::Display() {
	cGlobalBatchRenderer::instance()->Draw();

	if ( mCurrentView->NeedUpdate() )
		SetView( *mCurrentView );

	SwapBuffers();

	GLi->Clear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

	GetElapsedTime();

	CalculateFps();

	LimitFps();
}

void cWindow::ClipEnable( const Int32& x, const Int32& y, const Uint32& Width, const Uint32& Height ) {
	cGlobalBatchRenderer::instance()->Draw();
	GLi->Scissor( x, GetHeight() - Height - y, Width, Height );
	GLi->Enable( GL_SCISSOR_TEST );
}

void cWindow::ClipDisable() {
	cGlobalBatchRenderer::instance()->Draw();
	GLi->Disable( GL_SCISSOR_TEST );
}


void cWindow::ClipPlaneEnable( const Int32& x, const Int32& y, const Int32& Width, const Int32& Height ) {
	cGlobalBatchRenderer::instance()->Draw();
	GLi->Clip2DPlaneEnable( x, y, Width, Height );
}

void cWindow::ClipPlaneDisable() {
	cGlobalBatchRenderer::instance()->Draw();
	GLi->Clip2DPlaneDisable();
}

cClipboard * cWindow::GetClipboard() const {
	return mClipboard;
}

cInput * cWindow::GetInput() const {
	return mInput;
}

cCursorManager * cWindow::GetCursorManager() const {
	return mCursorManager;
}

Uint32 cWindow::PushResizeCallback( const WindowResizeCallback& cb ) {
	mNumCallBacks++;
	mCallbacks[ mNumCallBacks ] = cb;
	return mNumCallBacks;
}

void cWindow::PopResizeCallback( const Uint32& CallbackId ) {
	mCallbacks[ CallbackId ] = 0;
	mCallbacks.erase( mCallbacks.find(CallbackId) );
}

void cWindow::SendVideoResizeCb() {
	for ( std::map<Uint32, WindowResizeCallback>::iterator i = mCallbacks.begin(); i != mCallbacks.end(); i++ ) {
		i->second();
	}
}

void cWindow::LogSuccessfulInit( const std::string& BackendName ) {
	cLog::instance()->Write( "Engine Initialized Succesfully.\n\tBackend: " + BackendName + "\n\tGL Backend: " + GLi->VersionStr() + "\n\tGL Vendor: " + GLi->GetVendor() + "\n\tGL Renderer: " + GLi->GetRenderer() + "\n\tGL Version: " + GLi->GetVersion() + "\n\tGL Shading Language Version: " + GLi->GetShadingLanguageVersion() );
}

void cWindow::LogFailureInit( const std::string& ClassName, const std::string& BackendName ) {
	cLog::instance()->Write( "Error on " + ClassName + "::Init . Backend " + BackendName + " failed to start." );
}

std::string cWindow::Caption() {
	return mWindow.WindowConfig.Caption;
}

eeWindowContex cWindow::GetContext() const {
#if defined( EE_GLEW_AVAILABLE  ) && ( EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_MACOSX )
	return mWindow.Context;
#else
	return 0;
#endif
}

void cWindow::GetMainContext() {
#ifdef EE_GLEW_AVAILABLE

#if EE_PLATFORM == EE_PLATFORM_WIN
	mWindow.Context = wglGetCurrentContext();
#elif EE_PLATFORM == EE_PLATFORM_LINUX
	mWindow.Context = glXGetCurrentContext();
#elif EE_PLATFORM == EE_PLATFORM_MACOSX
	mWindow.Context = aglGetCurrentContext();
#endif

#endif
}

void cWindow::SetDefaultContext() {
#if defined( EE_GLEW_AVAILABLE ) && ( EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_LINUX )
	SetCurrentContext( mWindow.Context );
#endif
}

void cWindow::Minimize() {
	if ( NULL != mPlatform )
		mPlatform->MinimizeWindow();
}

void cWindow::Maximize() {
	if ( NULL != mPlatform )
		mPlatform->MaximizeWindow();
}

void cWindow::Hide() {
	if ( NULL != mPlatform )
		mPlatform->HideWindow();
}

void cWindow::Raise() {
	if ( NULL != mPlatform )
		mPlatform->RaiseWindow();
}

void cWindow::Show() {
	if ( NULL != mPlatform )
		mPlatform->ShowWindow();
}

void cWindow::Position( Int16 Left, Int16 Top ) {
	if ( NULL != mPlatform )
		mPlatform->MoveWindow( Left, Top );
}

eeVector2i cWindow::Position() {
	if ( NULL != mPlatform )
		return mPlatform->Position();

	return eeVector2i();
}

void cWindow::SetCurrentContext( eeWindowContex Context ) {
	if ( NULL != mPlatform )
		mPlatform->SetContext( Context );
}

void cWindow::CreatePlatform() {
	eeSAFE_DELETE( mPlatform );
	mPlatform = eeNew( Platform::cNullImpl, ( this ) );
}

void cWindow::SetCurrent() {
}

void cWindow::Center() {
	if ( Windowed() ) {
		Position( mWindow.DesktopResolution.Width() / 2 - mWindow.WindowConfig.Width / 2, mWindow.DesktopResolution.Height() / 2 - mWindow.WindowConfig.Height / 2 );
	}
}

}}