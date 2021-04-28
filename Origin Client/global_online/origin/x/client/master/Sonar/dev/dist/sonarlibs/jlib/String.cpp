#include <jlib/String.h>

#ifdef _WIN32
#include <jlib/windefs.h>
#else
#include <stdlib.h>
#endif
namespace jlib
{

std::string String::wideToUtf8(const std::wstring &wide)
{
	size_t cbOut = (wide.size () + 1) * 4;
	char *pmbstr = new char[cbOut];

#ifdef _WIN32
	WideCharToMultiByte (
		CP_UTF8, 
		0,
		wide.c_str (),
		(int) wide.size () + 1,
		pmbstr,
		(int) cbOut,
		NULL,
		NULL);
#else
	size_t result = wcstombs(pmbstr, wide.c_str (), cbOut - 1);
#endif

	std::string ret(pmbstr);

	delete pmbstr;
	return ret;
}

std::wstring String::utf8ToWide(const std::string &utf8)
{
	size_t cchOut = utf8.size () + 1;
	wchar_t *pwcstr = new wchar_t[cchOut + 1];
	
#ifdef _WIN32
	MultiByteToWideChar (
		CP_UTF8,
		0,
		utf8.c_str(),
		(int) utf8.size() + 1,
		pwcstr,
		(int) cchOut);
#else
	size_t result = mbstowcs (pwcstr, utf8.c_str (), cchOut); 
#endif
	std::wstring ret(pwcstr);
	delete pwcstr;
	return ret;
}

String::List String::tokenize(const std::wstring &str, int delim)
{
	size_t length = str.size ();
	size_t start = 0;

	List list;

	const wchar_t *pstr = str.c_str ();

	for (size_t index = 0; index < length; index ++)
	{
		if (*pstr == delim)
		{
			list.push_back (str.substr (start, (index - start)));
			start = index + 1;
		}
		pstr ++;
	}

	list.push_back (str.substr(start, length));

	return list;
}


}