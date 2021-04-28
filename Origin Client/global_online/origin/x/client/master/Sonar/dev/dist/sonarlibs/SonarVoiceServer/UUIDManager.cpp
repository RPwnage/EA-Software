#include "UUIDManager.h"

#ifdef _WIN32
#include <Rpc.h>
#else
#include <uuid/uuid.h>
#endif

#include <stdio.h>
#include <stdlib.h>

namespace sonar
{

UUIDManager::UUIDManager(int _index)
{
	m_index = _index;
}


const std::string UUIDManager::getPath(void)
{
	char *pstrHome = getenv ("SONAR_UUID_PATH");
	std::string path;
	char strIndex[256+1];
	sprintf (strIndex, "%d", m_index);

	if (pstrHome == NULL)
	{

#ifdef _WIN32
		path += getenv ("HOMEDRIVE");
		path += getenv ("HOMEPATH");
		path += "\\.sonar_uuid_";
		path += strIndex;
#else
		path += getenv ("HOME");
		path += "/.sonar_uuid_";
		path += strIndex;
#endif

	}
	else
	{
		path = pstrHome;
		path += "_";
		path += strIndex;
	}

	return path;
}

const std::string &UUIDManager::get()
{
	if (!m_currUUID.empty ())
	{
		return m_currUUID;
	}

	if (!read ())
	{
		generate ();
		write ();
	}

	return m_currUUID;

}

bool UUIDManager::read (void)
{
	FILE *file = fopen (getPath ().c_str (), "r");

	if (!file)
	{
		return false;
	}

	char lineBuffer[64 + 1];

	if (!fgets (lineBuffer, 64, file))
	{
		fclose (file);
		return false;
	}

	fclose (file);


#ifdef _WIN32
	UUID uuid;
	if (::UuidFromStringA ( (RPC_CSTR) lineBuffer, &uuid) != RPC_S_OK)
	{
		return false;
	}

#else
	uuid_t uu;
	if (uuid_parse(lineBuffer, uu) == -1)
	{
		return false;
	}
#endif

	m_currUUID = lineBuffer;

	return true;
}

bool UUIDManager::write (void)
{
	FILE *file = fopen (getPath ().c_str (), "w");

	if (!file)
	{
		return false;
	}

	fputs (m_currUUID.c_str (), file);
	fclose (file);
	return true;
}

bool UUIDManager::generate(void)
{
#ifdef _WIN32
	UCHAR *pstrUUID;
	UUID uuid;
	::UuidCreate (&uuid);
	::UuidToStringA (&uuid, &pstrUUID);

	m_currUUID = (char *) pstrUUID;

	::RpcStringFreeA (&pstrUUID);

#else
	char strTemp[64 + 1];
	uuid_t uu;
	uuid_generate(uu);
	uuid_unparse_lower (uu, strTemp); 
	m_currUUID = strTemp;
#endif
	
	return true;

}

}