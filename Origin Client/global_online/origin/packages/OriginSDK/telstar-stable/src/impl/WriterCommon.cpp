#include "stdafx.h"
#include "WriterCommon.h"
#include <stdio.h>
#include <string>

#ifdef ORIGIN_MAC
#define sprintf_s snprintf
#endif
bool Write(INodeDocument *doc, const char * name, const char * value)
{
	doc->AddChild(name);
	doc->SetValue(value);
	doc->Parent();

	return true;
}

bool Write(INodeDocument *doc, const char * name, int32_t value)
{
	char Buffer[32];
	sprintf_s(Buffer, 32, "%d", value);
	return Write(doc, name, Buffer);
}

bool Write(INodeDocument *doc, const char * name, uint32_t value)
{
	char Buffer[32];
	sprintf_s(Buffer, 32, "%u", value);
	return Write(doc, name, Buffer);
}

bool Write(INodeDocument *doc, const char * name, int64_t value)
{
	char Buffer[128];
	sprintf_s(Buffer, 128, "%lld", value);
	return Write(doc, name, Buffer);
}

bool Write(INodeDocument *doc, const char * name, uint64_t value)
{
	char Buffer[128];
	sprintf_s(Buffer, 128, "%llu", value);
	return Write(doc, name, Buffer);
}

bool Write(INodeDocument *doc, const char * name, OriginTimeT st)
{
	char Buffer[128];
#ifdef ORIGIN_PC
	TIME_ZONE_INFORMATION tzi;
	GetTimeZoneInformation(&tzi);
	sprintf_s(Buffer, 128, "%04d-%02d-%02dT%02d:%02d:%02d%+02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, (int)(tzi.Bias/60), (int)(tzi.Bias%60));
	return Write(doc, name, Buffer);
#else
    // Timezone information is not handled properly.
    sprintf(Buffer, "%04d-%02d-%02dT%02d:%02d:%02d%+02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, 0, 0);
	return Write(doc, name, Buffer);
#endif
}

bool Write(INodeDocument *doc, const char * name, float value)
{
	char Buffer[33];
	sprintf_s(Buffer, 33, "%.8g", value);
	return Write(doc, name, Buffer);
}
bool Write(INodeDocument *doc, const char * name, double value)
{
	char Buffer[65];
	sprintf_s(Buffer, 65, "%.18g", value);
	return Write(doc, name, Buffer);
}
bool Write(INodeDocument *doc, const char * name, bool value)
{
	char Buffer[32];
	sprintf_s(Buffer, 32, "%s", value ? "true" : "false");
	return Write(doc, name, Buffer);
}
bool Write(INodeDocument *doc, const char * name, Origin::AllocString &value)
{
	doc->AddChild(name);
	doc->SetValue(value.c_str());
	doc->Parent();

	return true;
}

bool WriteEnumValue(INodeDocument *doc, const char * name, const char *pValues[], int32_t data)
{
	doc->AddChild(name);
	doc->SetValue(pValues[data]);
	doc->Parent();

	return true;
}

void WriteEnumValueAttribute(INodeDocument *doc, const char * name, const char *pValues[], int32_t data)
{
	doc->AddAttribute(name, pValues[data]);
}
bool Write(INodeDocument *doc, const char *pValues[], int32_t data)
{
	return doc->SetValue(pValues[data]);
}

void WriteAttribute(INodeDocument *doc, const char * name, const char *pValues[], int32_t data)
{
	doc->AddAttribute(name, pValues[data]);
}

void WriteAttribute(INodeDocument *doc, const char * name, int32_t value)
{
	char Buffer[32];
	sprintf_s(Buffer, 32, "%d", value);
	WriteAttribute(doc, name, Buffer);
}

void WriteAttribute(INodeDocument *doc, const char * name, uint32_t value)
{
	char Buffer[32];
	sprintf_s(Buffer, 32, "%u", value);
	WriteAttribute(doc, name, Buffer);
}

void WriteAttribute(INodeDocument *doc, const char * name, int64_t value)
{
	char Buffer[128];
	sprintf_s(Buffer, 128, "%lld", value);
	WriteAttribute(doc, name, Buffer);
}

void WriteAttribute(INodeDocument *doc, const char * name, uint64_t value)
{
	char Buffer[128];
	sprintf_s(Buffer, 128, "%llu", value);
	WriteAttribute(doc, name, Buffer);
}

void WriteAttribute(INodeDocument *doc, const char * name, OriginTimeT st)
{
	char Buffer[128];
#ifdef ORIGIN_PC
	TIME_ZONE_INFORMATION tzi;
	GetTimeZoneInformation(&tzi);
	sprintf_s(Buffer, 128, "%04d-%02d-%02dT%02d:%02d:%02d%+02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, (int)(tzi.Bias/60), (int)(tzi.Bias%60));
	WriteAttribute(doc, name, Buffer);
#else
    sprintf(Buffer, "%04d-%02d-%02dT%02d:%02d:%02d%+02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, 0, 0);
    WriteAttribute(doc, name, Buffer);
#endif
}

void WriteAttribute(INodeDocument *doc, const char * name, float value)
{
	char Buffer[33];
	sprintf_s(Buffer, 33, "%.8g", value);
	WriteAttribute(doc, name, Buffer);
}

void WriteAttribute(INodeDocument *doc, const char * name, double value)
{
	char Buffer[65];
	sprintf_s(Buffer, 65, "%.16g", value);
	WriteAttribute(doc, name, Buffer);
}

void WriteAttribute(INodeDocument *doc, const char * name, Origin::AllocString & value)
{
	doc->AddAttribute(name, value.c_str());
}

void WriteAttribute(INodeDocument *doc, const char * name, const char * value)
{
	doc->AddAttribute(name, value);
}

void WriteAttribute(INodeDocument *doc, const char * name, bool value)
{
	char Buffer[32];
	sprintf_s(Buffer, 32, "%s", value ? "true" : "false");
	return WriteAttribute(doc, name, Buffer);
}

