#include "WriterCommon.h"

#if defined(_WIN32) || defined(_WIN64)
#pragma warning(disable:4996)
#endif

#include <stdio.h>

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
	sprintf(Buffer, "%d", value);
	return Write(doc, name, Buffer);
}

bool Write(INodeDocument *doc, const char * name, uint32_t value)
{
	char Buffer[32];
	sprintf(Buffer, "%u", value);
	return Write(doc, name, Buffer);
}

bool Write(INodeDocument *doc, const char * name, int64_t value)
{
	char Buffer[128];
	sprintf(Buffer, "%lld", value);
	return Write(doc, name, Buffer);
}

bool Write(INodeDocument *doc, const char * name, uint64_t value)
{
	char Buffer[128];
	sprintf(Buffer, "%llu", value);
	return Write(doc, name, Buffer);
}

bool Write(INodeDocument *doc, const char * name, OriginTimeT st)
{
	char Buffer[128];
	sprintf(Buffer, "%04d-%02d-%02dT%02d:%02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	return Write(doc, name, Buffer);
}

bool Write(INodeDocument *doc, const char * name, float value)
{
	char Buffer[33];
	sprintf(Buffer, "%.8g", value);
	return Write(doc, name, Buffer);
}
bool Write(INodeDocument *doc, const char * name, double value)
{
	char Buffer[65];
	sprintf(Buffer, "%.18g", value);
	return Write(doc, name, Buffer);
}
bool Write(INodeDocument *doc, const char * name, bool value)
{
	char Buffer[32];
	sprintf(Buffer, "%s", value ? "true" : "false");
	return Write(doc, name, Buffer);
}


bool Write(INodeDocument *doc, const char * name, QString &value)
{
	doc->AddChild(name);
	doc->SetValue(value.toUtf8().constData());
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

void WriteAttribute(INodeDocument *doc, const char * name, const char * value)
{
	doc->AddAttribute(name, value);
}

void WriteAttribute(INodeDocument *doc, const char * name, int32_t value)
{
	char Buffer[32];
	sprintf(Buffer, "%d", value);
	WriteAttribute(doc, name, Buffer);
}

void WriteAttribute(INodeDocument *doc, const char * name, uint32_t value)
{
	char Buffer[32];
	sprintf(Buffer, "%u", value);
	WriteAttribute(doc, name, Buffer);
}

void WriteAttribute(INodeDocument *doc, const char * name, int64_t value)
{
	char Buffer[128];
	sprintf(Buffer, "%lld", value);
	WriteAttribute(doc, name, Buffer);
}

void WriteAttribute(INodeDocument *doc, const char * name, uint64_t value)
{
	char Buffer[128];
	sprintf(Buffer, "%llu", value);
	WriteAttribute(doc, name, Buffer);
}

void WriteAttribute(INodeDocument *doc, const char * name, OriginTimeT st)
{
	char Buffer[128];
	sprintf(Buffer, "%04d-%02d-%02dT%02d:%02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	WriteAttribute(doc, name, Buffer);
}

void WriteAttribute(INodeDocument *doc, const char * name, float value)
{
	char Buffer[33];
	sprintf(Buffer, "%.8g", value);
	WriteAttribute(doc, name, Buffer);
}

void WriteAttribute(INodeDocument *doc, const char * name, double value)
{
	char Buffer[65];
	sprintf(Buffer, "%.16g", value);
	WriteAttribute(doc, name, Buffer);
}

void WriteAttribute(INodeDocument *doc, const char * name, QString & value)
{
	doc->AddAttribute(name, value.toUtf8().constData());
}

void WriteAttribute(INodeDocument *doc, const char * name, bool value)
{
	char Buffer[32];
	sprintf(Buffer, "%s", value ? "true" : "false");
	return WriteAttribute(doc, name, Buffer);
}

