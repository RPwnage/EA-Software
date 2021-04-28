#ifndef __WRITER_COMMON_H__
#define __WRITER_COMMON_H__

#include <OriginSDK/OriginTypes.h>
#include "XmlDocument.h"
#include "OriginSDKMemory.h"
#include "Common.h"

void WriteAttribute(INodeDocument *doc, const char * name, int32_t value);
void WriteAttribute(INodeDocument *doc, const char * name, uint32_t value);
void WriteAttribute(INodeDocument *doc, const char * name, int64_t value);
void WriteAttribute(INodeDocument *doc, const char * name, uint64_t value);
void WriteAttribute(INodeDocument *doc, const char * name, OriginTimeT value);
void WriteAttribute(INodeDocument *doc, const char * name, float value);
void WriteAttribute(INodeDocument *doc, const char * name, double value);
void WriteAttribute(INodeDocument *doc, const char * name, bool value);
void WriteAttribute(INodeDocument *doc, const char * name, const char * str);
void WriteAttribute(INodeDocument *doc, const char * name, Origin::AllocString & str);
void WriteEnumValueAttribute(INodeDocument *doc, const char * name, const char *pValues[], int data);

bool Write(INodeDocument *doc, const char * name, int32_t value);
bool Write(INodeDocument *doc, const char * name, uint32_t value);
bool Write(INodeDocument *doc, const char * name, int64_t value);
bool Write(INodeDocument *doc, const char * name, uint64_t value);
bool Write(INodeDocument *doc, const char * name, OriginTimeT value);
bool Write(INodeDocument *doc, const char * name, float value);
bool Write(INodeDocument *doc, const char * name, double value);
bool Write(INodeDocument *doc, const char * name, bool value);
bool Write(INodeDocument *doc, const char * name, const char * value);
bool Write(INodeDocument *doc, const char * name, Origin::AllocString & value);
bool WriteEnumValue(INodeDocument *doc, const char * name, const char *pValues[], int data);


#endif // __WRITER_COMMON_H__
