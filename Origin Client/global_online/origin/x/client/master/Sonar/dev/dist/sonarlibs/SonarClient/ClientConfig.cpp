#include <SonarClient/ClientConfig.h>
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace sonar
{

static double StringToDouble(const char *str)
{
	double ret;
	char *end;
	ret = strtod(str, &end);
	return ret;
}

static String DoubleToString(double value)
{
	String ret;
	char temp[256];

#ifdef _WIN32
	if (_snprintf(temp, 30, "%.20g", value) == -1)
#else
	if (snprintf(temp, 30, "%.20g", value) == -1)
#endif
	{
		return ret;
	}

	ret = temp;
	return ret;
}

CString ClientConfig::get(CString &name, CString &def)
{
	KeyValueMap::iterator iter = m_kvMap.find(name);
	if (iter == m_kvMap.end())
		return def;

	return iter->second;
}

float ClientConfig::get(CString &name, float def)
{
	KeyValueMap::iterator iter = m_kvMap.find(name);
	if (iter == m_kvMap.end())
		return def;

	return (float) StringToDouble(iter->second.c_str());
}

int ClientConfig::get(CString &name, int def)
{
	KeyValueMap::iterator iter = m_kvMap.find(name);
	if (iter == m_kvMap.end())
		return def;

	return (int) StringToDouble(iter->second.c_str());
}

bool ClientConfig::getBool(CString &name, bool def)
{
	KeyValueMap::iterator iter = m_kvMap.find(name);
	if (iter == m_kvMap.end())
		return def;

	if (iter->second == "true")
	{
		return true;
	}

	return false;
}

void ClientConfig::set(CString &name, CString &value)
{
	m_kvMap[name] = value;
}

void ClientConfig::set(CString &name, double value)
{
	set(name, DoubleToString(value));
}

void ClientConfig::set(CString &name, int value)
{
	set(name, DoubleToString(value));
}

void ClientConfig::set(CString &name, bool value)
{
	CString s = value ? "true": "false";
	set(name, s);
}

bool ClientConfig::save(CString &filePath)
{
	FILE *file = fopen(filePath.c_str(), "wb");

	if (!file)
	{
		return false;
	}

	fprintf(file, 
		"#=============================================\n"
		"#= GENERATED, MODIFY WITH CARE OR NOT AT ALL =\n"
		"#=============================================\n\n");

	for (KeyValueMap::iterator iter = m_kvMap.begin(); iter != m_kvMap.end(); iter ++)
	{
		fprintf(file, "%s:%s\n", iter->first.c_str(), iter->second.c_str());
	}


	fclose(file);

	return true;
}

bool ClientConfig::load(CString &filePath)
{
	FILE *file = fopen(filePath.c_str(), "rb");

	if (!file)
	{
		return false;
	}

	char lineBuffer[4096];

	while (fgets(lineBuffer, 4095, file))
	{

		size_t len = strlen(lineBuffer);

		if (len > 0 && (lineBuffer[len - 1] == '\r' || 	lineBuffer[len - 1] == '\n'))
			lineBuffer[len - 1] = '\0';

		if (len > 1 && (lineBuffer[len - 2] == '\r' || 	lineBuffer[len - 2] == '\n'))
			lineBuffer[len - 2] = '\0';

		String line = lineBuffer;

		size_t pos = line.find_first_of(L':');

		if (pos == String::npos)
		{
			continue;
		}

		String key = line.substr(0, pos);
		String value = line.substr(pos+1);

		m_kvMap[key] = value;
	}

	fclose(file);

	return true;
}

}