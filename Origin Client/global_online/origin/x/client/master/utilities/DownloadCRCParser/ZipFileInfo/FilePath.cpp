/**
CFilePath class implementation
*/

#include "FilePath.h"
#include "StringHelpers.h"


#include <windows.h>

#define IS_PATH_SEPARATOR(x) (x == '/' || x == '\\')
#define PATH_SEPARATOR '\\'

#if defined(_DEBUG)

//using namespace Core;

static QString TestFilePath()
{
	const QString names[] = { ".",
							 "..",
							 "./",
							 "C:\\SDK\\ptlib\\src\\ptclib\\pvidfile.cxx",
						     "C:\\SDK\\ptlib\\src\\ptclib\\pvidfile", //No extension
						     "C:\\SDK\\ptlib\\src\\ptclib\\pvidfile.", //No extension, but with dot
						     "\\SDK\\ptlib\\src\\ptclib\\pvidfile.cxx", //No drive
						     "C:\\SDK\\ptlib\\src\\ptclib\\", //No file 
							 "pvidfile.cxx"
						   };

	CFilePath path;

	SetCurrentDirectory(L"C:\\vivs\\");

	path = CFilePath::TempDirectory();

	for ( int i = 0; i < _countof(names); i++ )
	{
		path = QString(names[i]);

		QString vol = path.GetVolume();
		QString dir = path.GetDirectory();
		QString file = path.GetFileName();
		QString ext = path.GetFileExtension();
		bool isDir = path.IsDirectory();
		bool isAbs = path.IsAbsolute();
		bool exists = path.Exists();
		path.MakeAbsolute();
	}

	
	path = CFilePath::CurrentDirectory();
	path = CFilePath::TempDirectory();
	path = CFilePath::TempFileName();
	path = CFilePath::TempFileName("mytemp");
	path = CFilePath::TempFileName("mytemp", "c:\\tmp" );

	path = CFilePath::Absolutize(".", "afile.txt");
	path = CFilePath::Absolutize("c:\\vivs\\", "afile.txt");
	path = CFilePath::Absolutize("c:\\vivs\\", "../vivs/../afile.txt");
	path = CFilePath::Absolutize("c:\\vivs\\", "./afile.txt");
	
	path = CFilePath::Absolutize("c:\\vivs\\", "./afile/");

	return QString();
}

static const QString pepe = TestFilePath();

#endif

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
		//Fin the next slash
		int idx = mPath.indexOf(PATH_SEPARATOR, 2);

		if ( idx > 0 )
		{
			return mPath.mid(2, idx - 2);
		}
	}

	return QString();
}

QString CFilePath::GetDirectory() const
{
	if ( IsDirectory() )
		return mPath;

	QString dir;

	int idx = mPath.lastIndexOf( PATH_SEPARATOR );

	if ( idx >= 0 )
	{
		//Include slash
		dir = mPath.left( idx + 1 ); 
	}

	return dir;
}

QString CFilePath::GetFileName() const
{
	if ( IsDirectory() )
		return QString();

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

	return name;
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
		LPTSTR filePart = 0;
        int len = ::GetFullPathName((LPCWSTR)mPath.utf16(), 0, 0, &filePart);

		if ( len > 0 )
		{
			wchar_t* abs = new wchar_t[len+1];
			memset(abs, 0, (len+1) * sizeof(wchar_t));
            len = ::GetFullPathName((LPCWSTR)mPath.utf16(), len, abs, &filePart);
			mPath = QString::fromWCharArray(abs);
			delete[] abs;
		}
		else
		{
			//If GetFullPathName() fails is because mPath is invalid, so we clear it out
			mPath.clear();
		}
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
    return !IsEmpty() && (_waccess((LPCWSTR)mPath.utf16(), 00) == 0);
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
	CFilePath name;

	u_long len = ::GetCurrentDirectory(0, NULL);

	if ( len > 0 )
	{
		wchar_t* winPath = new wchar_t[len+1];
		memset(winPath, 0, (len+1) * sizeof(wchar_t));
		::GetCurrentDirectory(len, winPath);
		name.mPath = QString::fromWCharArray(winPath);
		name.AppendSlash();
		delete[] winPath;
	}

	return name;
}

CFilePath CFilePath::TempDirectory()
{
	CFilePath name;

	u_long len = ::GetTempPath(0, NULL);

	if ( len > 0 )
	{
		wchar_t* winPath = new wchar_t[len+1];
		::GetTempPath(len, winPath);
		name.mPath = QString::fromWCharArray(winPath);
		name.AppendSlash();
		delete[] winPath;
	}

	return name;
}

CFilePath CFilePath::TempFileName( const QString& prefix )
{
	return TempFileName( prefix, CurrentDirectory().ToString() );
}

CFilePath CFilePath::TempFileName( const QString& prefix, const QString& dir )
{
	wchar_t* fname = new wchar_t[MAX_PATH + 1];
	memset(fname, 0, (MAX_PATH + 1) * sizeof(wchar_t));
    UINT size = ::GetTempFileName((LPCWSTR)dir.utf16(), (LPCWSTR)prefix.utf16(), 0, fname);

	/**
	GetTempFileName() actually creates the file which (tempnam does not do that)
	So we better delete this immediately, it is up to the caller creating this file
	*/
	if ( size > 0 )
	{
		::DeleteFile(fname);
	}
	else
	{
		memset(fname, 0, (MAX_PATH + 1) * sizeof(wchar_t));
	}

	CFilePath tempFile(QString::fromWCharArray(fname));
	delete[] fname;

	return tempFile;
}

CFilePath CFilePath::Absolutize(const CFilePath& dir, const CFilePath& file)
{
	if ( file.IsAbsolute() )
		return file;

	CFilePath newPath = dir;
	newPath.MakeAbsolute();
	newPath = newPath.GetDirectory();

	newPath.mPath.append( file.mPath );
	newPath.MakeAbsolute();

	return newPath;
}

void CFilePath::ReplaceSlashes()
{
	mPath.replace('/', PATH_SEPARATOR);
}

void CFilePath::AppendSlash()
{
	if ( !IsEmpty() && mPath.right(1) != "\\" )
	{
		mPath.append(PATH_SEPARATOR);
	}
}

