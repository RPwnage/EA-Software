// CFilePath class to manage a file path and its parts
#ifndef _FILEPATH_CLASS_H_
#define _FILEPATH_CLASS_H_

#include <QString>
#include <QStringList>
#include <QChar>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Downloader
{

class ORIGIN_PLUGIN_API CFilePath
{
public:
	CFilePath();
	CFilePath( const QByteArray& path);
	CFilePath( const QString& path );
    CFilePath( const CFilePath & other );
	CFilePath( const char* path);
	~CFilePath();

    /**
	Get the drive/volume name
	*/
    QString GetVolume() const;

    /**
	Get the directory part
	*/
    QString GetDirectory() const;

	/**
	Get the file name including extension
	*/
	QString GetFileName() const;
	
	/**
	Get the file extension
	*/
	QString GetFileExtension() const;

	/**
	Test if the path is relative (does not begin with a drive or slash)
	*/
	bool IsRelative() const;

	/**
	Test if file is absolute (begins with a drive or slash)
	*/
	bool IsAbsolute() const;

	/**
	Build an absolute path from this relative path and using the current directory as base.
	If the path is not relative then it does nothing.
	*/
	bool MakeAbsolute();

	/**
	Test if initialized
	*/
	bool IsEmpty() const;

	/**
	Test if path refers to a directory (ends with slash)
	*/
	bool IsDirectory() const;

    /**
    Test if path is a file (i.e. not a directory).... useful for clear code
    */
    bool IsFile() const { return !IsDirectory(); }

	/**
	Test existance
	*/	
	bool Exists() const;

	/**
	Ensure this is a directory
	*/
	void AppendSlash();

	/**
	Get a string with the path representation
	*/
	QString ToString() const;

	/**
	Get the components of the file path.
	If simplify is indicated, '.' and '..' are resolved if possible
	*/
	void ToComponents(QStringList& components, bool simplify = false) const;

	/**
	Check if this directry is a subdir of 'path'
	*/
	bool IsSubdirOf( const CFilePath & path ) const;

	/**
	Operators
	*/
    CFilePath& operator=( const QString& str );
    CFilePath& operator=( const CFilePath & path );
	
    bool operator==( const QString& str ) const;
    bool operator==( const CFilePath & path ) const;
	bool operator!=( const QString& str ) const;
	bool operator!=( const CFilePath & path ) const;

	/**
	Test if the given cha is valid as part of a file path
	*/
    static bool IsValidChar( QChar c );

	/**
	Test if character is a path separator (forward or backward slash)
	*/
	static bool IsPathSeparator( QChar c );

	/**
	Test if the string contains valid characters
	*/
    static bool IsValid( const QString& str );

	/**
	Current working directory
	*/
	static CFilePath CurrentDirectory();

	/**
	Temp directory
	*/
	static CFilePath TempDirectory();

	/**
	Generate a temporary file name within a certain dir
	Only the three first chars of prefi are used
	*/
	static CFilePath TempFileName( const QString& prefix, const QString& dir );

	/**
	Concatenate two directories. 
	dir will be absolutized if required and the file name removed
	file must be a relative path, otherwise file is returned as is
	*/
	static CFilePath Absolutize(const CFilePath& dir, const CFilePath& file);
	
private:
	void ReplaceSlashes();
	
private:
	QString mPath;			/* The full file path */
};

} // namespace Downloader
} // namespace Origin

#endif

