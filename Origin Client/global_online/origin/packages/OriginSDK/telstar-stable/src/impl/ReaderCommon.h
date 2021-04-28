#ifndef __READER_COMMON_H__
#define __READER_COMMON_H__

#include <OriginSDK/OriginTypes.h>
#include "OriginMap.h"
#include "XmlDocument.h"
#include "OriginSDKMemory.h"
#include "Common.h"

bool Read(INodeDocument * doc, const char *name, int32_t &value);
bool Read(INodeDocument * doc, const char *name, uint32_t &value);
bool Read(INodeDocument * doc, const char *name, int64_t &value);
bool Read(INodeDocument * doc, const char *name, uint64_t &value);
bool Read(INodeDocument * doc, const char *name, OriginTimeT &value);
bool Read(INodeDocument * doc, const char *name, float &value);
bool Read(INodeDocument * doc, const char *name, double &value);
bool Read(INodeDocument * doc, const char *name, bool &value);
bool Read(INodeDocument * doc, const char *name, Origin::AllocString &value);
bool ReadEnumValue(INodeDocument * doc, const char *name, const char *pValues[], int Count, int &data);
bool ReadEnumValue(INodeDocument * doc, const char *name, const char *pValues[], lsx::EnumToIndexMap *map, int Count, int &data);


bool Read(INodeDocument * doc, int32_t &value);
bool Read(INodeDocument * doc, uint32_t &value);
bool Read(INodeDocument * doc, int64_t &value);
bool Read(INodeDocument * doc, uint64_t &value);
bool Read(INodeDocument * doc, OriginTimeT &value);
bool Read(INodeDocument * doc, float &value);
bool Read(INodeDocument * doc, double &value);
bool Read(INodeDocument * doc, bool &value);
bool Read(INodeDocument * doc, Origin::AllocString &value);
bool ReadEnumValue(INodeDocument * doc, const char *pValues[], int Count, int &data);
bool ReadEnumValue(INodeDocument * doc, const char *pValues[], lsx::EnumToIndexMap *map, int Count, int &data);


bool ReadAttribute(INodeDocument * doc, const char *name, int32_t &value);
bool ReadAttribute(INodeDocument * doc, const char *name, uint32_t &value);
bool ReadAttribute(INodeDocument * doc, const char *name, int64_t &value);
bool ReadAttribute(INodeDocument * doc, const char *name, uint64_t &value);
bool ReadAttribute(INodeDocument * doc, const char *name, OriginTimeT &value);
bool ReadAttribute(INodeDocument * doc, const char *name, float &value);
bool ReadAttribute(INodeDocument * doc, const char *name, double &value);
bool ReadAttribute(INodeDocument * doc, const char *name, bool &value);
bool ReadAttribute(INodeDocument * doc, const char *name, Origin::AllocString &value);
bool ReadEnumValueAttribute(INodeDocument * doc, const char *name, const char *pValues[], int Count, int &data);
bool ReadEnumValueAttribute(INodeDocument * doc, const char *name, const char *pValues[], lsx::EnumToIndexMap *map, int Count, int &data);

bool Read(const char *String, int32_t &value);
bool Read(const char *String, uint32_t &value);
bool Read(const char *String, int64_t &value);
bool Read(const char *String, uint64_t &value);
bool Read(const char *String, OriginTimeT &value);
bool Read(const char *String, float &value);
bool Read(const char *String, double &value);
bool Read(const char *String, bool &value);
bool Read(const char *String, Origin::AllocString &value);
bool ReadEnumValue(const char *pValue, const char *pValues[], int Count, int &data);
bool ReadEnumValue(const char *pValue, const char *pValues[], lsx::EnumToIndexMap *map, int Count, int &data);

Origin::AllocString GetPrefix(INodeDocument * doc, const char *nameSpace);
unsigned long GetHash(const char *pCommand);

#endif // __READER_COMMON_H__
