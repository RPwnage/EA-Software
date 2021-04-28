/**
CFilePath class implementation
*/

#include "FilePath.h"
#include "services/downloader/StringHelpers.h"
#include <QDir>
#include <QTemporaryFile>

#define IS_PATH_SEPARATOR(x) (x == '/' || x == '\\')
#define PATH_SEPARATOR QDir::separator()  // returns the native directory separator for the OS. '\' on win; '/' on mac/unix.

namespace Origin
{
namespace Downloader
{

CFilePath::CFilePath()
{
}

CFilePath::CFilePath( const QByteArray& path )
{
	mPath = path;
	ReplaceSlashes();
}

CFilePath::CFilePath( const QString& path )
{
	mPath = path;
	ReplaceSlashes();
}

CFilePath::CFilePath( const char* path)
{
	mPath = path;
	ReplaceSlashes();
}

CFilePath::CFilePath( const CFilePath& other )
{
	mPath = other.ToString();
}

CFilePath::~CFilePath()
{
}

QString CFilePath::GetVolume() const
{
	//Empty
	if ( mPath.length() < 2 )
		return QString();

	//Local drive
	if ( mPath.at(1) == ':' )
		return mPath.left(1);

	//UNC path
	if ( IsPathSeparator(mPath.at(0)) && IsPathSeparator(mPath.at(1)) )
	{
		//Find the next slash
		int idx = mPath.indexOf(PATH_SEPARATOR, 2);

		if ( idx > 0 )
		{
			return mPath.mid(2, idx - 2);
		}
	}

	// This is probably missing a volume or is a Mac (no drive letters)
	return QString("/");
}

// Returns the absolute path of the parent directory of mPath.  Path ends with a directory separator.
QString CFilePath::GetDirectory() const
{
	if ( IsDirectory() )
		return mPath;

	QFileInfo fileInfo(mPath);

    return QDir::toNativeSeparators(fileInfo.dir().absolutePath()) + QDir::separator();
}

// Returns the name of the file, excluding the path.
QString CFilePath::GetFileName() const
{
	if ( IsDirectory() )
		return QString();

    QFileInfo fileInfo(mPath);

    return fileInfo.fileName();	
}

QString CFilePath::GetFileExtension() const
{
	QString name = GetFileName();

	int idx = name.lastIndexOf('.');

	if ( idx >= 0 )
	{
		name = name.right(name.length() - idx - 1 );
	}
	else
	{
		name.clear();
	}

	return name;
}

bool CFilePath::IsRelative() const
{
	return !IsAbsolute();
}

bool CFilePath::IsAbsolute() const
{
	if ( mPath.isEmpty() )
		return false;

	//Starts with slash (UNC included)
	if ( IsPathSeparator( mPath.at(0) ) )
		return true;

	//Drive specifier
	if ( mPath.size() > 1 && mPath.at(1) == ':' )
		return true;

	return false;
}

bool CFilePath::MakeAbsolute()
{
    bool isDir = (IsEmpty() || IsDirectory());

    if ( IsEmpty() )
    {
        *this = CurrentDirectory();
    }
    else
    {
        QDir qPath(mPath);
		mPath = QDir::toNativeSeparators(qPath.absolutePath());
    }

    if ( isDir )
    {
        AppendSlash();
    }

    return !IsEmpty();
}

bool CFilePath::IsEmpty() const
{
	return mPath.isEmpty();
}

bool CFilePath::IsDirectory() const
{
	if ( IsEmpty() )
		return false;

	if ( IsPathSeparator( mPath.right(1).at(0) ) )
		return true;

	//special '.' and '..' notations
	QString name;

	int idx = mPath.lastIndexOf( PATH_SEPARATOR );

	if ( idx == -1 )
	{
		idx = 0;
	}
	else
	{
		idx++;
	}

	if ( idx >= 0 )
	{
		name = mPath.right( mPath.length() - idx );
	}

	if ( name == "." || name == ".." )
		return true;

	return false;
}

bool CFilePath::Exists() const
{
    if(IsEmpty())
        return false;
    if(IsDirectory())
    {
        QDir qPath(mPath);
        return qPath.exists();
    }
    // if not empty, and not a dir, must be a file
    else
    {
        QFile qPath(mPath);
        return qPath.exists();
    }
}


QString CFilePath::ToString() const
{
	return mPath;
}

void CFilePath::ToComponents(QStringList& components, bool simplify) const
{
	int iCur = 0;
	int iStr = 0;

	components.clear();

	while ( iStr < mPath.length())
	{
		//Go to the first separator
		while ( iStr < mPath.length() && !IS_PATH_SEPARATOR(mPath[iStr]) )
		{
			iStr++;
		}

		components.append(mPath.mid(iCur, iStr-iCur));

		//Skip separators
		while ( iStr < mPath.length() && IS_PATH_SEPARATOR(mPath[iStr]) )
		{
			iStr++;
		}

		iCur = iStr;
	}

	if ( simplify )
	{
		QStringList::const_iterator it = components.constBegin();
		QStringList newPath;

		while ( it != components.constEnd() )
		{
			bool append = true;
			const QString& comp = *it;

			if ( comp == "." )
			{
				append = false;
			}
			else if ( comp == ".." )
			{
				if ( newPath.count() > 0 )
				{
					newPath.removeLast();
					append = false;
				}
			}

			if ( append )
			{
				newPath.append( comp );
			}

			it++;
		}

		components = newPath;
	}
}

bool CFilePath::IsSubdirOf( const CFilePath & path ) const
{
	//Both paths should be abolute, otherwise there is no way to tell
	if ( IsRelative() || path.IsRelative() )
		return false;

	QStringList path1, path2;

	ToComponents(path1,true);
	path.ToComponents(path2,true);

	//this path should have less components, subdir does not include same directory
	if ( path1.size() <= path2.size() )
		return false;

	QStringList::const_iterator it1, it2;

	it1 = path1.constBegin();
	it2 = path2.constBegin();

	bool result = true;

	while ( (it1 != path1.constEnd()) && (it2 != path2.constEnd()) )
	{
		const QString& cmp1 = *it1++;
		const QString& cmp2 = *it2++;

		if ( cmp1.compare(cmp2, Qt::CaseInsensitive) != 0 )
		{
			result = false;
			break;
		}
	}

	return result;
}

CFilePath& CFilePath::operator=( const QString& str )
{
	mPath = str;
	ReplaceSlashes();
	return *this;
}

CFilePath& CFilePath::operator=( const CFilePath& path )
{
	mPath = path.mPath;
	return *this;
}

bool CFilePath::operator==( const QString& str ) const
{
	CFilePath other(str);
	return *this == other;
}

bool CFilePath::operator==( const CFilePath& other ) const
{
	//Do a case insensitive compare
	if ( mPath.compare(other.mPath, Qt::CaseInsensitive) == 0 )
		return true;

	//Normalize both
	CFilePath path1 = *this;
	CFilePath path2 = other;

	path1.MakeAbsolute();
	path2.MakeAbsolute();

	//Do a case insensitive compare
	if ( path1.mPath.compare(path2.mPath, Qt::CaseInsensitive) == 0 )
		return true;

	return false;
}

bool CFilePath::operator!=( const QString& str ) const
{
	return !(*this == str);
}

bool CFilePath::operator!=( const CFilePath & path ) const
{
	return !(*this == path);
}

static const QString InvalidCharsStr = "/\\:*?\"<>|";

bool CFilePath::IsValidChar( QChar c )
{
	return InvalidCharsStr.indexOf(c) == -1;
}

bool CFilePath::IsPathSeparator( QChar c )
{
	return c == '\\' || c == '/';
}

bool CFilePath::IsValid( const QString& str )
{
	QString::const_iterator iter = InvalidCharsStr.constBegin();

	while(iter != InvalidCharsStr.constEnd())
	{
		if(str.indexOf((*iter)) != -1)
		{
			return false;
		}
		iter++;
	}

	return true;
}

CFilePath CFilePath::CurrentDirectory()
{
    CFilePath name(QDir::currentPath());
    name.AppendSlash();

    return name;
}

CFilePath CFilePath::TempDirectory()
{
    CFilePath name(QDir::tempPath());
    name.AppendSlash();

    return name;
}

CFilePath CFilePath::TempFileName( const QString& prefix, const QString& dir )
{
    // create the template for the temporary file name
    QString tempPath = dir + prefix + "XXXXXX.tmp";
    QTemporaryFile tempFile(tempPath);
    tempFile.open();
    CFilePath name(tempFile.fileName());
    tempFile.close();

    return name;
}

CFilePath CFilePath::Absolutize(const CFilePath& dir, const CFilePath& file)
{
	if ( file.IsAbsolute() )
		return file;

	CFilePath newPath(dir);
	newPath.MakeAbsolute();
	newPath = newPath.GetDirectory();

	newPath.mPath.append( file.mPath );
	newPath.MakeAbsolute();

	return newPath;
}

void CFilePath::ReplaceSlashes()
{
	mPath.replace('/', PATH_SEPARATOR);
	mPath.replace('\\', PATH_SEPARATOR);
}

void CFilePath::AppendSlash()
{
	QString sSeparator(PATH_SEPARATOR);
	if ( !IsEmpty() && mPath.right(1) != sSeparator )
	{
		mPath.append(PATH_SEPARATOR);
	}
}

} // namespace Downloader
} // namespace Origin

