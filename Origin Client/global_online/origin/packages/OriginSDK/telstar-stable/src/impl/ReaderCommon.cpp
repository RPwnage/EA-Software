#include "stdafx.h"
#include "ReaderCommon.h"
#include <stdio.h>
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
    {
#ifdef ORIGIN_PC
		value = _stricmp(String, "false") == 0 ? false : true;
#elif defined ORIGIN_MAC
        value = strcasecmp(String, "false") == 0 ? false : true;
#endif
    }
	return true;
}

bool Read(const char *String, Origin::AllocString &value)
{
	if(String != NULL)
		value = String;
	return true;
}

bool Read(INodeDocument * doc, int32_t &value)
{
	bool ret = true;
	const char* textVal = doc->GetValue();
	if (textVal)
	{
		ret = Read(textVal, value);
	}
	return ret;
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
	bool ret = true;
	const char* textVal = doc->GetValue();
	if (textVal)
	{
		ret = Read(textVal, value);
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
	bool ret = true;
	const char* textVal = doc->GetValue();
	if (textVal)
	{
		ret = Read(textVal, value);
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
	bool ret = true;
	const char* textVal = doc->GetValue();
	if (textVal)
	{
		ret = Read(textVal, value);
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
	bool ret = true;
	const char* textVal = doc->GetValue();
	if (textVal)
	{
		ret = Read(textVal, value);
	}
	return ret;
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
	bool ret = true;
	const char* textVal = doc->GetValue();
	if (textVal)
	{
		ret = Read(textVal, value);
	}
	return ret;
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
	bool ret = true;
	const char* textVal = doc->GetValue();
	if (textVal)
	{
		ret = Read(textVal, value);
	}
	return ret;
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
	bool ret = true;
	const char* textVal = doc->GetValue();
	if (textVal)
	{
		ret = Read(textVal, value);
	}
	return ret;
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

bool Read(INodeDocument * doc, Origin::AllocString &value)
{
	bool ret = true;
	const char* textVal = doc->GetValue();
	if (textVal)
	{
		ret = Read(textVal, value);
	}
    return ret;
}

bool Read(INodeDocument * doc, const char *name, Origin::AllocString &value)
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

bool ReadEnumValue(const char *pValue, const char *pValues[], lsx::EnumToIndexMap * map, int Count, int &data)
{
	if(pValue != NULL)
	{
		for(int i=0; i<Count; i++)
		{
			if(strcmp(pValue, pValues[i]) == 0)
			{
				data = GetEnumFromIndex(map, Count, i, 0);
				return true;
			}
		}
	}
	return false;
}

bool ReadEnumValue(INodeDocument * doc, const char *name, const char *pValues[], lsx::EnumToIndexMap * map, int Count, int &data)
{
	bool ret = false;
	if(doc->FirstChild(name))
	{
		const char* text = doc->GetValue();
		if (text)
		{
		    ret = ReadEnumValue(text, pValues, map, Count, data);
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

bool ReadAttribute(INodeDocument * doc, const char *name, Origin::AllocString &value)
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

bool ReadEnumValueAttribute(INodeDocument * doc, const char *name, const char *pValues[], lsx::EnumToIndexMap *map, int Count, int &data)
{
	bool ret = false;
	if(doc->GetAttribute(name))
	{
		const char* text = doc->GetAttributeValue();
		if (text)
		{
			ret = ReadEnumValue(text, pValues, map, Count, data);
		}
	}
	return ret;
}

Origin::AllocString GetPrefix(INodeDocument * doc, const char *nameSpace)
{
	Origin::AllocString prefix = "";

	if(doc->FirstAttribute())
	{
		do
		{
#ifdef ORIGIN_PC
			if(_stricmp(doc->GetAttributeValue(), nameSpace) == 0)
#elif defined ORIGIN_MAC
            if(strcasecmp(doc->GetAttributeValue(), nameSpace) == 0)
#endif
			{
				if(strlen(doc->GetAttributeName())>6)
					prefix = doc->GetAttributeName() + 6; //"xmlns:".Length
				return prefix;
			}
		}while(doc->NextAttribute());
	}
	return prefix;
}

unsigned long GetHash(const char *pName)
{
	if (pName == nullptr)
		return 0;

    return Hash::MurmurHash2(pName, strlen(pName), 0x34123549);
}
