#include "projectdirectorytree.hpp"
#include <algorithm>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/luapattern.hpp>

ProjectDirectoryTree::ProjectDirectoryTree( const std::string& path,
											std::shared_ptr<ThreadPool> threadPool ) :
	mPath( path ), mPool( threadPool ), mIsReady( false ), mIgnoreMatcher( path ) {
	FileSystem::dirAddSlashAtEnd( mPath );
}

void ProjectDirectoryTree::scan( const ProjectDirectoryTree::ScanCompleteEvent& scanComplete,
								 const std::vector<std::string>& acceptedPattern,
								 const bool& ignoreHidden ) {
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
	mPool->run(
		[&, acceptedPattern, ignoreHidden] {
#endif
			Lock l( mFilesMutex );
			mDirectories.push_back( mPath );
			if ( !acceptedPattern.empty() ) {
				std::vector<std::string> files;
				std::vector<std::string> names;
				std::vector<LuaPattern> patterns;
				for ( auto& strPattern : acceptedPattern )
					patterns.emplace_back( LuaPattern( strPattern ) );
				std::set<std::string> info;
				getDirectoryFiles( files, names, mPath, info, ignoreHidden, mIgnoreMatcher );
				size_t namesCount = names.size();
				bool found;
				for ( size_t i = 0; i < namesCount; i++ ) {
					found = false;
					for ( auto& pattern : patterns ) {
						if ( pattern.matches( names[i] ) ) {
							found = true;
							break;
						}
					}
					if ( found ) {
						mFiles.emplace_back( std::move( files[i] ) );
						mNames.emplace_back( std::move( names[i] ) );
					}
				}
			} else {
				std::set<std::string> info;
				getDirectoryFiles( mFiles, mNames, mPath, info, ignoreHidden, mIgnoreMatcher );
			}
			mIsReady = true;
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN && !defined( __EMSCRIPTEN_PTHREADS__ )
			if ( scanComplete )
				scanComplete( *this );
#endif
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
		},
		[scanComplete, this] {
			if ( scanComplete )
				scanComplete( *this );
		} );
#endif
}

std::shared_ptr<FileListModel> ProjectDirectoryTree::fuzzyMatchTree( const std::string& match,
																	 const size_t& max ) const {
	std::multimap<int, int, std::greater<int>> matchesMap;
	std::vector<std::string> files;
	std::vector<std::string> names;
	for ( size_t i = 0; i < mNames.size(); i++ )
		matchesMap.insert( { String::fuzzyMatch( mNames[i], match ), i } );
	for ( auto& res : matchesMap ) {
		if ( names.size() < max ) {
			names.emplace_back( mNames[res.second] );
			files.emplace_back( mFiles[res.second] );
		}
	}
	return std::make_shared<FileListModel>( files, names );
}

std::shared_ptr<FileListModel> ProjectDirectoryTree::matchTree( const std::string& match,
																const size_t& max ) const {
	std::vector<std::string> files;
	std::vector<std::string> names;
	std::string lowerMatch( String::toLower( match ) );
	for ( size_t i = 0; i < mNames.size(); i++ ) {
		if ( String::toLower( mNames[i] ).find( lowerMatch ) != std::string::npos ) {
			names.emplace_back( mNames[i] );
			files.emplace_back( mFiles[i] );
			if ( max == names.size() )
				return std::make_shared<FileListModel>( files, names );
		}
	}
	return std::make_shared<FileListModel>( files, names );
}

void ProjectDirectoryTree::asyncFuzzyMatchTree( const std::string& match, const size_t& max,
												ProjectDirectoryTree::MatchResultCb res ) const {
	mPool->run( [&, match, max, res]() { res( fuzzyMatchTree( match, max ) ); }, []() {} );
}

void ProjectDirectoryTree::asyncMatchTree( const std::string& match, const size_t& max,
										   ProjectDirectoryTree::MatchResultCb res ) const {
	mPool->run( [&, match, max, res]() { res( matchTree( match, max ) ); }, []() {} );
}

std::shared_ptr<FileListModel> ProjectDirectoryTree::asModel( const size_t& max ) const {
	if ( mNames.empty() )
		return std::make_shared<FileListModel>( std::vector<std::string>(),
												std::vector<std::string>() );
	size_t rmax = eemin( mNames.size(), max );
	std::vector<std::string> files( rmax );
	std::vector<std::string> names( rmax );
	for ( size_t i = 0; i < rmax; i++ ) {
		files[i] = mFiles[i];
		names[i] = mNames[i];
	}
	return std::make_shared<FileListModel>( files, names );
}

size_t ProjectDirectoryTree::getFilesCount() const {
	return mFiles.size();
}

const std::vector<std::string>& ProjectDirectoryTree::getFiles() const {
	return mFiles;
}

const std::vector<std::string>& ProjectDirectoryTree::getDirectories() const {
	return mDirectories;
}

bool ProjectDirectoryTree::isFileInTree( const std::string& filePath ) const {
	return std::find( mFiles.begin(), mFiles.end(), filePath ) != mFiles.end();
}

bool ProjectDirectoryTree::isDirInTree( const std::string& dirTree ) const {
	std::string dir( FileSystem::fileRemoveFileName( dirTree ) );
	FileSystem::dirAddSlashAtEnd( dir );
	return std::find( mDirectories.begin(), mDirectories.end(), dirTree ) != mDirectories.end();
}

void ProjectDirectoryTree::getDirectoryFiles( std::vector<std::string>& files,
											  std::vector<std::string>& names,
											  std::string directory,
											  std::set<std::string> currentDirs,
											  const bool& ignoreHidden,
											  const IgnoreMatcherManager& ignoreMatcher ) {
	currentDirs.insert( directory );
	std::string localDirPath( directory.substr(
		ignoreMatcher.foundMatch() ? ignoreMatcher.getPath().size() : mPath.size() ) );
	std::vector<std::string> pathFiles =
		FileSystem::filesGetInPath( directory, false, false, ignoreHidden );
	for ( auto& file : pathFiles ) {
		std::string fullpath( directory + file );
		std::string localpath( localDirPath + file );
		if ( ignoreMatcher.foundMatch() && ignoreMatcher.match( localpath ) )
			continue;
		if ( FileSystem::isDirectory( fullpath ) ) {
			fullpath += FileSystem::getOSSlash();
			FileInfo dirInfo( fullpath, true );
			if ( dirInfo.isLink() ) {
				fullpath = dirInfo.linksTo();
				FileSystem::dirAddSlashAtEnd( fullpath );
				if ( currentDirs.find( fullpath ) == currentDirs.end() )
					continue;
				mDirectories.push_back( fullpath );
			} else {
				mDirectories.push_back( fullpath );
			}
			IgnoreMatcherManager dirMatcher( fullpath );
			getDirectoryFiles( files, names, fullpath, currentDirs, ignoreHidden,
							   dirMatcher.foundMatch() ? dirMatcher : ignoreMatcher );
		} else {
			files.emplace_back( fullpath );
			names.emplace_back( file );
		}
	}
}
