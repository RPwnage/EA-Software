#include "ReaderCommon.h"

#if defined(_WIN32) || defined(_WIN64)
#pragma warning(disable:4996)
#endif

#include <stdio.h>
#include <string.h>
#include <string>
#include "MurmurHash.h"


bool Read(const char *String, int32_t &value)
{
	if(String != NULL)
		return sscanf(String, "%d", &value) == 1;
	return false;
}

bool Read(const char *String, uint32_t &value)
{
	if(String != NULL)
		return sscanf(String, "%u", &value) == 1;
	return false;
}

bool Read(const char *String, int64_t &value)
{
	if(String != NULL)
		return sscanf(String, "%lld", &value) == 1;
	return false;
}

bool Read(const char *String, uint64_t &value)
{
	if(String != NULL)
		return sscanf(String, "%llu", &value) == 1;
	return false;
}

bool Read(const char *String, OriginTimeT &st)
{
	st.wYear = 0;
	st.wMonth = 0;
	st.wDayOfWeek = 0;
	st.wDay = 0;
	st.wHour = 0;
	st.wMinute = 0;
	st.wSecond = 0;
	st.wMilliseconds = 0;

	if(String != NULL)
	{
		int ret = sscanf(String, "%hu-%hu-%huT%hu:%hu:%hu", &st.wYear, &st.wMonth, &st.wDay, &st.wHour, &st.wMinute, &st.wSecond);
		return ret>=3;
	}
	return false;
}

bool Read(const char *String, float &value)
{
	if(String != NULL)
		return sscanf(String, "%f", &value) == 1;
	return false;
}

bool Read(const char *String, double &value)
{
	if(String != NULL)
		return sscanf(String, "%lf", &value) == 1;
	return false;
}

bool Read(const char *String, bool &value)
{
	if(String != NULL)
#ifdef _WIN32
		value = _stricmp(String, "false") == 0 ? false : true;
#else
        value = strcasecmp(String, "false") == 0 ? false : true;
#endif
	return true;
}

bool Read(const char *String, QString &value)
{
	if(String != NULL)
		value = QString::fromUtf8(String);
	return true;
}

bool Read(INodeDocument * doc, int32_t &value)
{
	const char* textVal = doc->GetValue();
	if (textVal)
	{
		return Read(textVal, value);
	}
	return false;
}
bool Read(INodeDocument * doc, const char *name, int32_t &value)
{
	bool ret = false;
	if(doc->FirstChild(name))
	{
		ret = Read(doc, value);
	}
	doc->Parent();
	return ret;
}

bool Read(INodeDocument * doc, uint32_t &value)
{
	const char* textVal = doc->GetValue();
	if (textVal)
	{
		return Read(textVal, value);
	}
	return false;
}
bool Read(INodeDocument * doc, const char *name, uint32_t &value)
{
	bool ret = false;
	if(doc->FirstChild(name))
	{
		ret = Read(doc, value);
	}
	doc->Parent();
	return ret;
}
bool Read(INodeDocument * doc, int64_t &value)
{
	const char* textVal = doc->GetValue();
	if (textVal)
	{
		return Read(textVal, value);
	}
	return false;
}

bool Read(INodeDocument * doc, const char *name, int64_t &value)
{
	bool ret = false;
	if(doc->FirstChild(name))
	{
		ret = Read(doc, value);
	}
	doc->Parent();
	return ret;
}

bool Read(INodeDocument * doc, uint64_t &value)
{
	const char* textVal = doc->GetValue();
	if (textVal)
	{
		return Read(textVal, value);
	}
	return false;
}
bool Read(INodeDocument * doc, const char *name, uint64_t &value)
{
	bool ret = false;
	if(doc->FirstChild(name))
	{
		ret=Read(doc, value);
	}
	doc->Parent();
	return ret;
}

bool Read(INodeDocument * doc, OriginTimeT &value)
{
	const char* textVal = doc->GetValue();
	if (textVal)
	{
		return Read(textVal, value);
	}
	return false;
}

bool Read(INodeDocument * doc, const char *name, OriginTimeT &value)
{
	bool ret = false;
	if(doc->FirstChild(name))
	{
		ret = Read(doc, value);
	}
	doc->Parent();
	return ret;
}

bool Read(INodeDocument * doc, float &value)
{
	const char* textVal = doc->GetValue();
	if (textVal)
	{
		return Read(textVal, value);
	}
	return false;
}

bool Read(INodeDocument * doc, const char *name, float &value)
{
	bool ret = false;
	if(doc->FirstChild(name))
	{
		ret = Read(doc, value);
	}
	doc->Parent();
	return ret;
}

bool Read(INodeDocument * doc, double &value)
{
	const char* textVal = doc->GetValue();
	if (textVal)
	{
		return Read(textVal, value);
	}
	return false;
}

bool Read(INodeDocument * doc, const char *name, double &value)
{
	bool ret = false;
	if(doc->FirstChild(name))
	{
		ret = Read(doc, value);
	}
	doc->Parent();
	return ret;
}

bool Read(INodeDocument * doc, bool &value)
{
	const char* textVal = doc->GetValue();
	if (textVal)
	{
		return Read(textVal, value);
	}
	return false;
}

bool Read(INodeDocument * doc, const char *name, bool &value)
{
	bool ret = false;
	if(doc->FirstChild(name))
	{
		ret = Read(doc, value);
	}
	doc->Parent();
	return ret;
}
bool Read(INodeDocument * doc, QString &value)
{
	const char* textVal = doc->GetValue();
	if (textVal)
	{
		return Read(textVal, value);
	}
	return false;
}

bool Read(INodeDocument * doc, const char *name, QString &value)
{
	bool ret = false;
	if(doc->FirstChild(name))
	{
		ret = Read(doc, value);
	}
	doc->Parent();
	return ret;
}

bool ReadEnumValue(const char *pValue, const char *pValues[], int Count, int &data)
{
	if(pValue != NULL)
	{
		for(int i=0; i<Count; i++)
		{
			if(strcmp(pValue, pValues[i]) == 0)
			{
				data = i;
				return true;
			}
		}
	}
	return false;
}

bool ReadEnumValue(INodeDocument * doc, const char *name, const char *pValues[], int Count, int &data)
{
	bool ret = false;
	if(doc->FirstChild(name))
	{
		const char* text = doc->GetValue();
		if (text)
		{
			ret = ReadEnumValue(text, pValues, Count, data);
		}
	}
	doc->Parent();
	return ret;
}

bool ReadEnumValue(const char *pValue, const char *pValues[], std::map<int, int> & IndexToEnumMap, int Count, int &data)
{
	if(pValue != NULL)
	{
		for(int i=0; i<Count; i++)
		{
			if(strcmp(pValue, pValues[i]) == 0)
			{
				data = IndexToEnumMap[i];
				return true;
			}
		}
	}
	return false;
}

bool ReadEnumValue(INodeDocument * doc, const char *name, const char *pValues[], std::map<int, int> & IndexToEnumMap, int Count, int &data)
{
	bool ret = false;
	if(doc->FirstChild(name))
	{
		const char* text = doc->GetValue();
		if (text)
		{
			ret = ReadEnumValue(text, pValues, IndexToEnumMap, Count, data);
		}
	}
	doc->Parent();
	return ret;
}


bool ReadAttribute(INodeDocument * doc, const char *name, int32_t &value)
{
	bool ret = false;
	if(doc->GetAttribute(name))
	{
		const char* textVal = doc->GetAttributeValue();
		if (textVal)
		{
			ret = Read(textVal, value);
		}
	}
	return ret;
}

bool ReadAttribute(INodeDocument * doc, const char *name, uint32_t &value)
{
	bool ret = false;
	if(doc->GetAttribute(name))
	{
		const char* textVal = doc->GetAttributeValue();
		if (textVal)
		{
			ret = Read(textVal, value);
		}
	}
	return ret;
}

bool ReadAttribute(INodeDocument * doc, const char *name, int64_t &value)
{
	bool ret = false;
	if(doc->GetAttribute(name))
	{
		const char* textVal = doc->GetAttributeValue();
		if (textVal)
		{
			ret = Read(textVal, value);
		}
	}
	return ret;
}

bool ReadAttribute(INodeDocument * doc, const char *name, uint64_t &value)
{
	bool ret = false;
	if(doc->GetAttribute(name))
	{
		const char* textVal = doc->GetAttributeValue();
		if (textVal)
		{
			ret = Read(textVal, value);
		}
	}
	return ret;
}

bool ReadAttribute(INodeDocument * doc, const char *name, OriginTimeT &value)
{
	bool ret = false;
	if(doc->GetAttribute(name))
	{
		const char* textVal = doc->GetAttributeValue();
		if (textVal)
		{
			ret = Read(textVal, value);
		}
	}
	return ret;
}

bool ReadAttribute(INodeDocument * doc, const char *name, float &value)
{
	bool ret = false;
	if(doc->GetAttribute(name))
	{
		const char* textVal = doc->GetAttributeValue();
		if (textVal)
		{
			ret = Read(textVal, value);
		}
	}
	return ret;
}

bool ReadAttribute(INodeDocument * doc, const char *name, double &value)
{
	bool ret = false;
	if(doc->GetAttribute(name))
	{
		const char* textVal = doc->GetAttributeValue();
		if (textVal)
		{
			ret = Read(textVal, value);
		}
	}
	return ret;
}

bool ReadAttribute(INodeDocument * doc, const char *name, bool &value)
{
	bool ret = false;
	if(doc->GetAttribute(name))
	{
		const char* textVal = doc->GetAttributeValue();
		if (textVal)
		{
			ret = Read(textVal, value);
		}
	}
	return ret;
}

bool ReadAttribute(INodeDocument * doc, const char *name, QString &value)
{
	bool ret = false;
	if(doc->GetAttribute(name))
	{
		const char* textVal = doc->GetAttributeValue();
		if (textVal)
		{
			ret = Read(textVal, value);
		}
	}
	return ret;
}

bool ReadEnumValueAttribute(INodeDocument * doc, const char *name, const char *pValues[], int Count, int &data)
{
	bool ret = false;
	if(doc->GetAttribute(name))
	{
		const char* text = doc->GetAttributeValue();
		if (text)
		{
			ret = ReadEnumValue(text, pValues, Count, data);
		}
	}
	return ret;
}

bool ReadEnumValueAttribute(INodeDocument * doc, const char *name, const char *pValues[], std::map<int, int> & IndexToEnumMap, int Count, int &data)
{
	bool ret = false;
	if(doc->GetAttribute(name))
	{
		const char* text = doc->GetAttributeValue();
		if (text)
		{
			ret = ReadEnumValue(text, pValues, IndexToEnumMap, Count, data);
		}
	}
	return ret;
}

unsigned long GetHash(const char *pName)
{
	return Hash::MurmurHash2(pName, strlen(pName), 0x34123549);
}

bool IsXml(const char *pCommand)
{
	if (!pCommand)
		return false;

	return pCommand[0] == '<';
}

std::string GetPrefix(INodeDocument * doc, const char *nameSpace)
{
	return "";
}

