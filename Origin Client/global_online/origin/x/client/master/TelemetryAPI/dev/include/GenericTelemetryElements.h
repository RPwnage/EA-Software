///////////////////////////////////////////////////////////////////////////////
// GenericTelemetryElements.h
//
// Implements all generic Telemetry error codes, type enums and data structures
//
// Owner: Origin Dev Support
// Copyright (c) Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef GENERICTELEMETRYELEMENTS_H
#define GENERICTELEMETRYELEMENTS_H

#include "EABase/eabase.h"
#include "EASTL/map.h"
#include "EASTL/string.h"

#define kMAX_USERNAME  (128)
#define kMAX_NID       (32)
#define kMAX_TIMESTAMP (32)


#pragma pack(push, 1)

union TelemetryDataFormatType 
{
    int8_t i8;
    int16_t  i16;
    uint16_t u16;
    int32_t  i32;
    uint32_t u32;
    int64_t  i64;
    uint64_t u64;
    const char8_t* s8;
    const char16_t* s16;
    const char32_t* s32;
};


#define TYPE_I8 int8_t
#define TYPE_I16 int16_t
#define TYPE_U16 uint16_t
#define TYPE_I32 int32_t
#define TYPE_U32 uint32_t
#define TYPE_I64 int64_t
#define TYPE_U64 uint64_t
#define TYPE_S8 char8_t*
#define TYPE_S16 char16_t* 
#define TYPE_S32 char32_t*

enum TelemetryDataFormatTypes
{
    TYPEID_END = 0,
    TYPEID_I8,
    TYPEID_I16,
    TYPEID_U16,
    TYPEID_I32,
    TYPEID_U32,
    TYPEID_I64,
    TYPEID_U64,
    TYPEID_S8,
    TYPEID_S16,
    TYPEID_S32
};


enum TELEMETRY_SEND_SETTING // must match TelemetrySendSettingStrings
{
    SEND_TELEMETRY_SETTING_NOT_SET,
    SEND_TELEMETRY_SETTING_CRITICAL_ONLY,
    SEND_TELEMETRY_SETTING_ALL
};


enum TELEMETRY_UNDERAGE_SETTING // must match TelemetryUnderageSettingStrings
{
    TELEMETRY_UNDERAGE_NOT_SET,
    TELEMETRY_UNDERAGE_TRUE,
    TELEMETRY_UNDERAGE_FALSE
};


struct TelemetryFileAttributItem
{
    TelemetryDataFormatTypes type;

    TYPE_U64 valueSize; // in bytes
    char8_t value;
};

struct TelemetryFileChunk 
{
    TYPE_U32 metricID;
    TYPE_U32 metricGroupID;

    TYPE_U64 timeStampUTC;

    TYPE_U32 attributeCount;
    TYPE_U32 totalAttributeSize;
};

struct TelemetryCheckSum
{
    uint64_t A;
    uint64_t B;
    uint64_t C;
    uint64_t D;
};

// Please keep this enum in sync with TELEMETRY_STATUS_STRINGS in TelemetryEvent.cpp
enum OriginTelemetrySubscriberStatus
{
    TELEMETRY_SUBSCRIBER_STATUS_NOT_SET
    ,TELEMETRY_SUBSCRIBER_STATUS_NON_SUBSCRIBER
    ,TELEMETRY_SUBSCRIBER_STATUS_ACTIVE
    ,TELEMETRY_SUBSCRIBER_STATUS_TRIAL
    ,TELEMETRY_SUBSCRIBER_STATUS_LAPSED
};


class TelemetryEventListener
{
public:
    virtual void processEvent(const TYPE_U64 timestamp, const TYPE_S8 module, const TYPE_S8 group, const TYPE_S8 string, const eastl::map<eastl::string8, TelemetryFileAttributItem*>& data) = 0;
};

#pragma pack(pop)


#endif // TELEMETRYERRORS_H