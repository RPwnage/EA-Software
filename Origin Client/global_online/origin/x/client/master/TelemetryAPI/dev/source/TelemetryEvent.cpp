///////////////////////////////////////////////////////////////////////////////
// TelemetryEvent.h
//
// A class to encapsulate a telemetry event sent by the client.
//
// Created by Origin DevSupport
// Copyright (c) 2015 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////



#include "TelemetryEvent.h"
#include "EbisuMetrics.h"
#include "TelemetryLogger.h"

#include "GenericTelemetryElements.h" 

#include "Utilities.h"

#include "TelemetrySDK/telemetryapi.h"

#include "EASTL/shared_ptr.h"
#include "EAStdC/EADateTime.h"
#include "EAStdC/EAString.h"
#include "EAAssert/eaassert.h"


namespace OriginTelemetry
{
// This is the maximum size of a single telemetry event serialized into a single char buffer.
// Our max size is 8K, so I've set it to that, but it might need to be bumped up to accommodate the header.
// Currently we don't send anything even close to 4k(the old limit) so it shouldn't matter unless we add a new really big event.
static const uint64_t MAX_TOTAL_ATTRIBUTE_SIZE = 8 * 1024;
uint32_t TelemetryEvent::s_eventCounter = 0;
uint32_t TelemetryEvent::s_criticalCounter = 0;
uint32_t TelemetryEvent::s_nonCriticalCounter = 0;


namespace TelemetrySubscriptionStrings
{
    // Keep this in sync with OriginTelemetrySubscriberStatus in GenericTelemetryElements.h
    static const char* TELEMETRY_STATUS_STRINGS[] =
    {
        "0-NotSet"
        , "1-NonSubscriber"
        , "2-ActiveSubscriber"
        , "3-TrialSubscriber"
        , "4-LapsedSubscriber"
    };
}


TelemetryEvent::TelemetryEvent(const TelemetryDataFormatTypes *format, va_list vl) :
    m_eventBufferSharedPtr(),
    m_eventBufferSize(0),
    m_isCritical(NOT_SET),
    m_eventCounter(0),
    m_criticalTypeCounter(0),
    mSubscriberStatus(TELEMETRY_SUBSCRIBER_STATUS_NOT_SET)
{
    TelemetryDataFormatType t;
    CREATE_TELEMETRY_EVENT_ERRORS res=CREATE_TELEMETRY_EVENT_NO_ERRORS;

    TelemetryFileChunk *newChunk=NULL;

    size_t maxBufferSize = MAX_TOTAL_ATTRIBUTE_SIZE + sizeof(TelemetryFileChunk);

    //We create a max size buffer to ensure the event will fit, because we don't know the size at this point.
    char8_t* totalMem = reinterpret_cast<char8_t*>(malloc( maxBufferSize)); // preallocate an attribute buffer
    memset(totalMem, 0, maxBufferSize);

    newChunk = reinterpret_cast<TelemetryFileChunk*>(totalMem);

    newChunk->timeStampUTC = EA::StdC::GetTime() / UINT64_C(1000000000);    // in UTC seconds
    
    newChunk->attributeCount=0;
    newChunk->totalAttributeSize=0;
    newChunk->metricID = 0;
        

    char8_t *attribPtr = reinterpret_cast<char8_t*> (totalMem + sizeof(TelemetryFileChunk));

    for( int i = 0; format[i] != 0; ++i ) 
    {
        switch( format[i] ) 
        {
        case    TYPEID_I8:
            t.i8 = static_cast<int8_t>(va_arg( vl, TYPE_I32)); // note: fetching as 32-bit value to account for type promotion
            res=StoreAttribute(newChunk, attribPtr, TYPEID_I8, t);
            break;

        case    TYPEID_I16:
            t.i16 = static_cast<int16_t>(va_arg( vl, TYPE_I32)); // note: fetching as 32-bit value to account for type promotion
            res=StoreAttribute(newChunk, attribPtr, TYPEID_I16, t);
            break;

        case    TYPEID_U16:
            t.u16 = static_cast<uint16_t>(va_arg( vl, TYPE_U32)); // note: fetching as 32-bit value to account for type promotion
            res=StoreAttribute(newChunk, attribPtr, TYPEID_U16, t);
            break;

        case    TYPEID_I32:
            t.i32 = va_arg( vl, TYPE_I32);
            res=StoreAttribute(newChunk, attribPtr, TYPEID_I32, t);
            break;

        case    TYPEID_U32:
            t.u32 = va_arg( vl, TYPE_U32);

            // metric ID
            if(i==0)
                newChunk->metricID = t.u32;
            else
                // or metric group ID
                if(i==1)
                    newChunk->metricGroupID = t.u32;
                else
                    res=StoreAttribute(newChunk, attribPtr, TYPEID_U32, t);

            break;

        case    TYPEID_I64:
            t.i64 = va_arg( vl, TYPE_I64);          
            res=StoreAttribute(newChunk, attribPtr, TYPEID_I64, t);
            break;

        case    TYPEID_U64:
            t.u64 = va_arg( vl, TYPE_U64);
            res=StoreAttribute(newChunk, attribPtr, TYPEID_U64, t);
            break;

        case    TYPEID_S8:
            t.s8 = va_arg( vl, TYPE_S8);
            if(t.s8==NULL)
                t.s8="";
            res=StoreAttribute(newChunk, attribPtr, TYPEID_S8, t);
            break;

        case    TYPEID_S16:
            t.s16 = va_arg( vl, TYPE_S16);
            if(t.s16==NULL)
                t.s16=(char16_t*)L"";
            res=StoreAttribute(newChunk, attribPtr, TYPEID_S16, t);
            break;

        case    TYPEID_S32:           
            t.s32 = va_arg( vl, TYPE_S32);
            if(t.s32==NULL)
                t.s32=(char32_t*)L"  ";
            res=StoreAttribute(newChunk, attribPtr, TYPEID_S32, t);
            break;

        default:
            res=CREATE_TELEMETRY_EVENT_ERROR_UNKNOWN_ATTRIB_TYPE;
            break;
        }

        if( res != CREATE_TELEMETRY_EVENT_NO_ERRORS)
        {
            //Error processing this event so log the error and exit.
            // This will return an empty TelemetryEvent that the
            // telemetry buffer will reject
            eastl::string8 errorStr;
            errorStr.append_sprintf("Failed creating a TelemetryEvent for metric id %u due to an error parsing the parameters. Error = %d", newChunk->metricID, res);
            TelemetryLogger::GetTelemetryLogger().logTelemetryError(errorStr);
            EA_FAIL_MSG( errorStr.c_str() );

            free(totalMem);
            return;
        }
    }

    ///Attach the total event counter as well as the specific critical or non-critical counter to the event.
    m_isCritical = TelemetryCritical[ newChunk->metricID ] ? CRITICAL: NON_CRITICAL; 
    m_eventCounter = ++s_eventCounter;
    if ( m_isCritical )
        m_criticalTypeCounter = ++s_criticalCounter;
    else
        m_criticalTypeCounter = ++s_nonCriticalCounter;

    //Create a new buffer that the shared pointer points to, that is exactly the right size and copy the buffer in, then free the temp buffer
    m_eventBufferSize = sizeof(TelemetryFileChunk)+newChunk->totalAttributeSize;
    m_eventBufferSharedPtr.reset(new char8_t[m_eventBufferSize]); 
    memcpy(m_eventBufferSharedPtr.get(), totalMem, m_eventBufferSize);
    free(totalMem);
}


TelemetryEvent::CREATE_TELEMETRY_EVENT_ERRORS TelemetryEvent::StoreAttribute(TelemetryFileChunk *newChunk, char8_t *&attribMemoryPtr, TelemetryDataFormatTypes type, TelemetryDataFormatType t)
{
    uint64_t size = 0;
    char8_t const* data = NULL;

    switch (type)
    {
    case TYPEID_S8:
        size = (t.s8) ? EA::StdC::Strlen(t.s8) * sizeof(char8_t) : 0;
        data = reinterpret_cast<char8_t const*>(t.s8);
        break;
    case TYPEID_S16:
        size = (t.s16) ? EA::StdC::Strlen(t.s16) * sizeof(char16_t) : 0;
        data = reinterpret_cast<char8_t const*>(t.s16);
        break;
    case TYPEID_S32:
        size = (t.s32) ? EA::StdC::Strlen(t.s32) * sizeof(char32_t) : 0;
        data = reinterpret_cast<char8_t const*>(t.s32);
        break;

    case TYPEID_I8:
        size = sizeof(t.i8);
        data = reinterpret_cast<char8_t const*>(&t.i8);
        break;
    case TYPEID_I16:
        size = sizeof(t.i16);
        data = reinterpret_cast<char8_t const*>(&t.i16);
        break;
    case TYPEID_U16:
        size = sizeof(t.u16);
        data = reinterpret_cast<char8_t const*>(&t.u16);
        break;
    case TYPEID_I32:
        size = sizeof(t.i32);
        data = reinterpret_cast<char8_t const*>(&t.i32);
        break;
    case TYPEID_U32:
        size = sizeof(t.u32);
        data = reinterpret_cast<char8_t const*>(&t.u32);
        break;
    case TYPEID_I64:
        size = sizeof(t.i64);
        data = reinterpret_cast<char8_t const*>(&t.i64);
        break;
    case TYPEID_U64:
        size = sizeof(t.u64);
        data = reinterpret_cast<char8_t* const>(&t.u64);
        break;

    default:
        return CREATE_TELEMETRY_EVENT_ERROR_UNKNOWN_ATTRIB_TYPE;
        break;

    }

    uint64_t totalSize = size;
    totalSize += sizeof(TelemetryFileAttributItem);

    if (newChunk->totalAttributeSize + totalSize > MAX_TOTAL_ATTRIBUTE_SIZE)
        return CREATE_TELEMETRY_EVENT_ERROR_TOO_MANY_ATTRIBUTES;
    else
    {
        ((TelemetryFileAttributItem*)attribMemoryPtr)->type = type;
        ((TelemetryFileAttributItem*)attribMemoryPtr)->valueSize = size;
        memcpy(&((TelemetryFileAttributItem*)attribMemoryPtr)->value, data, (size_t)((TelemetryFileAttributItem*)attribMemoryPtr)->valueSize);

        newChunk->totalAttributeSize += (uint32_t)totalSize;
        newChunk->attributeCount++;
        attribMemoryPtr += totalSize;
    }

    return CREATE_TELEMETRY_EVENT_NO_ERRORS;
}

void TelemetryEvent::deserializeToEvent3( TelemetryApiEvent3T& event3)
{
    //eastl::string8 strModule, strGroup, strString;
    char8_t* totalMem = m_eventBufferSharedPtr.get();

    TelemetryFileChunk *chunkHeadPtr = reinterpret_cast<TelemetryFileChunk*>(totalMem);
    char8_t* attributePtr = reinterpret_cast<char8_t*>( reinterpret_cast<char8_t*>(chunkHeadPtr) + sizeof(TelemetryFileChunk));        

    char8_t** MetricListAccessor=(char8_t**)METRIC_LIST[ chunkHeadPtr->metricID ];

    uint32_t Module=0;
    uint32_t Group=0;
    uint32_t String=0;

    if((*MetricListAccessor)!=NULL)
    {
        //strModule = (*MetricListAccessor);
        size_t len=eastl::min<uint32_t>(sizeof(uint32_t), (uint32_t) EA::StdC::Strlen(*MetricListAccessor));
        NonQTOriginServices::Utilities::FourChars_2_UINT32((char8_t*)&Module, (char8_t*)*MetricListAccessor, len);
    }

    if((*MetricListAccessor)!=NULL)
        MetricListAccessor++;

    if((*MetricListAccessor)!=NULL)
    {
        //strGroup = (*MetricListAccessor);
        size_t len=eastl::min<uint32_t>(sizeof(uint32_t), (uint32_t) EA::StdC::Strlen(*MetricListAccessor));
        NonQTOriginServices::Utilities::FourChars_2_UINT32((char8_t*)&Group, (char8_t*)*MetricListAccessor, len);
    }

    if((*MetricListAccessor)!=NULL)
        MetricListAccessor++;

    if((*MetricListAccessor)!=NULL)
    {
        //strString = (*MetricListAccessor);
        size_t len=eastl::min<uint32_t>(sizeof(uint32_t), (uint32_t) EA::StdC::Strlen(*MetricListAccessor));
        NonQTOriginServices::Utilities::FourChars_2_UINT32((char8_t*)&String, (char8_t*)*MetricListAccessor, len);
    }

    if((*MetricListAccessor)!=NULL)
        MetricListAccessor++;

    TelemetryApiInitEvent3(&event3, Module, Group, String);

    for(uint32_t i=0; i<chunkHeadPtr->attributeCount; i++)
    {
        switch( ((TelemetryFileAttributItem*)attributePtr)->type ) 
        {
            case TYPEID_I8:
                {
                    TelemetryDataFormatType tmp;
                    GetAttribute(&((TelemetryFileAttributItem*)attributePtr)->value, reinterpret_cast<char8_t*>(&tmp), ((TelemetryFileAttributItem*)attributePtr)->valueSize);
                    //EA::StdC::Fprintf(file, "%s=\"%i\" ",                (*MetricListAccessor)==NULL ?"":*MetricListAccessor, tmp.i8 );
                    
                    if((*MetricListAccessor)!=NULL)
                    {
                        uint32_t tmpUINT32=0;
                        size_t len=eastl::min<uint32_t>(sizeof(uint32_t), (uint32_t) EA::StdC::Strlen(*MetricListAccessor));
                        NonQTOriginServices::Utilities::FourChars_2_UINT32((char8_t*)&tmpUINT32, (char8_t*)*MetricListAccessor, len);
                        TelemetryApiEncAttributeChar(&event3, tmpUINT32, tmp.i8);                    
                    }
                                        
                    if((*MetricListAccessor)!=NULL)
                        MetricListAccessor++;                    
                }
                break;

            case TYPEID_I16:
                {
                    TelemetryDataFormatType tmp;
                    GetAttribute(&((TelemetryFileAttributItem*)attributePtr)->value, reinterpret_cast<char8_t*>(&tmp), ((TelemetryFileAttributItem*)attributePtr)->valueSize);
                    //EA::StdC::Fprintf(file, "%s=\"%i\" ",                (*MetricListAccessor)==NULL ?"":*MetricListAccessor, tmp.i16 );
                    if((*MetricListAccessor)!=NULL)
                    {
                        uint32_t tmpUINT32=0;
                        size_t len=eastl::min<uint32_t>(sizeof(uint32_t), (uint32_t) EA::StdC::Strlen(*MetricListAccessor));
                        NonQTOriginServices::Utilities::FourChars_2_UINT32((char8_t*)&tmpUINT32, (char8_t*)*MetricListAccessor, len);
                        TelemetryApiEncAttributeInt(&event3, tmpUINT32, tmp.i16);                    
                    }

                    if((*MetricListAccessor)!=NULL)
                        MetricListAccessor++;    

                }
                break;

            case TYPEID_U16:
                {
                    TelemetryDataFormatType tmp;
                    GetAttribute(&((TelemetryFileAttributItem*)attributePtr)->value, reinterpret_cast<char8_t*>(&tmp), ((TelemetryFileAttributItem*)attributePtr)->valueSize);
                    //EA::StdC::Fprintf(file, "%s=\"%u\" ",                (*MetricListAccessor)==NULL ?"":*MetricListAccessor, tmp.u16 );
                    if((*MetricListAccessor)!=NULL)
                    {
                        uint32_t tmpUINT32=0;
                        size_t len=eastl::min<uint32_t>(sizeof(uint32_t), (uint32_t) EA::StdC::Strlen(*MetricListAccessor));
                        NonQTOriginServices::Utilities::FourChars_2_UINT32((char8_t*)&tmpUINT32, (char8_t*)*MetricListAccessor, len);
                        TelemetryApiEncAttributeInt(&event3, tmpUINT32, tmp.u16);                    
                    }

                    if((*MetricListAccessor)!=NULL)
                        MetricListAccessor++;                    
                }
                break;

            case TYPEID_I32:
                {
                    TelemetryDataFormatType tmp;
                    GetAttribute(&((TelemetryFileAttributItem*)attributePtr)->value, reinterpret_cast<char8_t*>(&tmp), ((TelemetryFileAttributItem*)attributePtr)->valueSize);
                    //EA::StdC::Fprintf(file, "%s=\"%li\" ",                (*MetricListAccessor)==NULL ?"":*MetricListAccessor, tmp.i32 );
                    if((*MetricListAccessor)!=NULL)
                    {
                        uint32_t tmpUINT32=0;
                        size_t len=eastl::min<uint32_t>(sizeof(uint32_t), (uint32_t) EA::StdC::Strlen(*MetricListAccessor));
                        NonQTOriginServices::Utilities::FourChars_2_UINT32((char8_t*)&tmpUINT32, (char8_t*)*MetricListAccessor, len);
                        TelemetryApiEncAttributeInt(&event3, tmpUINT32, tmp.i32);                    
                    }

                    if((*MetricListAccessor)!=NULL)
                        MetricListAccessor++;                    
                }
                break;

            case TYPEID_U32:
                {
                    TelemetryDataFormatType tmp;
                    GetAttribute(&((TelemetryFileAttributItem*)attributePtr)->value, reinterpret_cast<char8_t*>(&tmp), ((TelemetryFileAttributItem*)attributePtr)->valueSize);
                    //EA::StdC::Fprintf(file, "%s=\"%lu\" ",                (*MetricListAccessor)==NULL ?"":*MetricListAccessor, tmp.u32 );
                    if((*MetricListAccessor)!=NULL)
                    {
                        uint32_t tmpUINT32=0;
                        size_t len=eastl::min<uint32_t>(sizeof(uint32_t), (uint32_t) EA::StdC::Strlen(*MetricListAccessor));
                        NonQTOriginServices::Utilities::FourChars_2_UINT32((char8_t*)&tmpUINT32, (char8_t*)*MetricListAccessor, len);
                        TelemetryApiEncAttributeInt(&event3, tmpUINT32, tmp.u32);                    
                    }

                    if((*MetricListAccessor)!=NULL)
                        MetricListAccessor++;                    
                }
                break;

            case TYPEID_I64:
                {
                    TelemetryDataFormatType tmp;
                    GetAttribute(&((TelemetryFileAttributItem*)attributePtr)->value, reinterpret_cast<char8_t*>(&tmp), ((TelemetryFileAttributItem*)attributePtr)->valueSize);
                    //EA::StdC::Fprintf(file, "%s=\"%I64i\" ",                (*MetricListAccessor)==NULL ?"":*MetricListAccessor, tmp.i64 );
                    if((*MetricListAccessor)!=NULL)
                    {
                        uint32_t tmpUINT32=0;
                        size_t len=eastl::min<uint32_t>(sizeof(uint32_t), (uint32_t) EA::StdC::Strlen(*MetricListAccessor));
                        NonQTOriginServices::Utilities::FourChars_2_UINT32((char8_t*)&tmpUINT32, (char8_t*)*MetricListAccessor, len);
                        TelemetryApiEncAttributeLong(&event3, tmpUINT32, tmp.i64);                    
                    }

                    if((*MetricListAccessor)!=NULL)
                        MetricListAccessor++;                    
                }
                break;

            case TYPEID_U64:
                {
                    TelemetryDataFormatType tmp;
                    GetAttribute(&((TelemetryFileAttributItem*)attributePtr)->value, reinterpret_cast<char8_t*>(&tmp), ((TelemetryFileAttributItem*)attributePtr)->valueSize);
                    //EA::StdC::Fprintf(file, "%s=\"%I64u\" ",                (*MetricListAccessor)==NULL ?"":*MetricListAccessor, tmp.u64 );
                    if((*MetricListAccessor)!=NULL)
                    {
                        uint32_t tmpUINT32=0;
                        size_t len=eastl::min<uint32_t>(sizeof(uint32_t), (uint32_t) EA::StdC::Strlen(*MetricListAccessor));
                        NonQTOriginServices::Utilities::FourChars_2_UINT32((char8_t*)&tmpUINT32, (char8_t*)*MetricListAccessor, len);
                        TelemetryApiEncAttributeLong(&event3, tmpUINT32, tmp.u64);                    
                    }

                    if((*MetricListAccessor)!=NULL)
                        MetricListAccessor++;                    
                }
                break;

            case TYPEID_S8:
                {
                    uint64_t s=((TelemetryFileAttributItem*)attributePtr)->valueSize+1;
                    char8_t* tmp = (char8_t*)malloc((size_t)s);
                    memset(tmp, 0, (size_t)s);
                    GetAttribute(&((TelemetryFileAttributItem*)attributePtr)->value, (char8_t*)tmp, ((TelemetryFileAttributItem*)attributePtr)->valueSize);
                    //EA::StdC::Fprintf(file, "%s=\"%s\" ",                (*MetricListAccessor)==NULL ?"":*MetricListAccessor,  (char8_t*)tmp );

                    if((*MetricListAccessor)!=NULL)
                    {
                        uint32_t tmpUINT32=0;
                        size_t len=eastl::min<uint32_t>(sizeof(uint32_t), (uint32_t) EA::StdC::Strlen(*MetricListAccessor));
                        NonQTOriginServices::Utilities::FourChars_2_UINT32((char8_t*)&tmpUINT32, (char8_t*)*MetricListAccessor, len);
                        TelemetryApiEncAttributeString(&event3, tmpUINT32, tmp);                    
                        if (s > TELEMETRY3_EVENTSTRINGSIZE_DEFAULT)
                        {
                            eastl::string8 errorStr;
                            errorStr.append_sprintf("String size %ull Bigger than TELEMETRY3_EVENTSTRINGSIZE_DEFAULT %d", s, TELEMETRY3_EVENTSTRINGSIZE_DEFAULT);
                            TelemetryLogger::GetTelemetryLogger().logTelemetryError(errorStr);
                            EA_FAIL_MSG(errorStr.c_str());
                        }
                    }

                    free((void*)tmp);
                    if((*MetricListAccessor)!=NULL)
                        MetricListAccessor++;                    
                }
                break;

            case TYPEID_S16:
                {
                    uint64_t s=((TelemetryFileAttributItem*)attributePtr)->valueSize+1;
                    char8_t* tmp = (char8_t*)malloc((size_t)s);
                    memset(tmp, 0, (size_t)s);
                    GetAttribute(&((TelemetryFileAttributItem*)attributePtr)->value, (char8_t*)tmp, ((TelemetryFileAttributItem*)attributePtr)->valueSize);
                    //EA::StdC::Fprintf(file, "%S=\"%s\" ",                (*MetricListAccessor)==NULL ?"":*MetricListAccessor,  (char8_t*)tmp );

                    // not supported

                    free((void*)tmp);
                    if((*MetricListAccessor)!=NULL)
                        MetricListAccessor++;                    
                }                
                break;

            case TYPEID_S32:           
                {
                    uint64_t s=((TelemetryFileAttributItem*)attributePtr)->valueSize+1;
                    char8_t* tmp = (char8_t*)malloc((size_t)s);
                    memset(tmp, 0, (size_t)s);
                    GetAttribute(&((TelemetryFileAttributItem*)attributePtr)->value, (char8_t*)tmp, ((TelemetryFileAttributItem*)attributePtr)->valueSize);
                    //EA::StdC::Fprintf(file, "%S=\"%S\" ",                (*MetricListAccessor)==NULL ?"":*MetricListAccessor,  (char8_t*)tmp );

                    // not supported

                    free((void*)tmp);
                    if((*MetricListAccessor)!=NULL)
                        MetricListAccessor++;                    
                }                
                break;

                default:
                break;
        }

        attributePtr+=((TelemetryFileAttributItem*)attributePtr)->valueSize;
        attributePtr+= sizeof(TelemetryFileAttributItem);

    }
    ///Send the total event counter and then send either critical or non-critical counter depending on which the event is.
    TelemetryApiEncAttributeInt(&event3, 'dstn',m_eventCounter);
    if ( m_isCritical )
        TelemetryApiEncAttributeInt(&event3, 'dscn', m_criticalTypeCounter);
    else
        TelemetryApiEncAttributeInt(&event3, 'dsnn', m_criticalTypeCounter);

    //Add the subscriber status to every event
    TelemetryApiEncAttributeInt(&event3, 'subs', mSubscriberStatus);
}


uint64_t TelemetryEvent::getTimeStampUTC() const
{
    if(m_eventBufferSize < sizeof(TelemetryFileChunk))
        return 0;

    TelemetryFileChunk *headPointer = reinterpret_cast<TelemetryFileChunk *>(m_eventBufferSharedPtr.get());
    return headPointer->timeStampUTC;
}

uint32_t TelemetryEvent::getMetricID() const
{
    if(m_eventBufferSize < sizeof(TelemetryFileChunk))
        return 0;

    TelemetryFileChunk *headPointer = reinterpret_cast<TelemetryFileChunk *>(m_eventBufferSharedPtr.get());
    return headPointer->metricID;
}

eastl::map<eastl::string8, TelemetryFileAttributItem*> TelemetryEvent::getAttributeMap() const
{
    eastl::map<eastl::string8, TelemetryFileAttributItem*> attributeMap;
    char8_t* totalMem = m_eventBufferSharedPtr.get();

    TelemetryFileChunk *chunkHeadPtr = reinterpret_cast<TelemetryFileChunk*>(totalMem);
    char8_t* attributePtr = reinterpret_cast<char8_t*>( reinterpret_cast<char8_t*>(chunkHeadPtr) + sizeof(TelemetryFileChunk));        

    char8_t** MetricListAccessor=(char8_t**)METRIC_LIST[ chunkHeadPtr->metricID ];

    //Skip Module Group String.
    if((*MetricListAccessor)!=NULL)
        MetricListAccessor++;

    if((*MetricListAccessor)!=NULL)
        MetricListAccessor++;

    if((*MetricListAccessor)!=NULL)
        MetricListAccessor++;

    for(uint32_t i=0; i<chunkHeadPtr->attributeCount; i++)
    {
        if((*MetricListAccessor) != NULL)
        {
            attributeMap[*MetricListAccessor] = (TelemetryFileAttributItem*)attributePtr;
        
            MetricListAccessor++;
            attributePtr+=((TelemetryFileAttributItem*)attributePtr)->valueSize;
            attributePtr+= sizeof(TelemetryFileAttributItem);
        }
        else
        {
            break;
        }
    }
        
    return attributeMap;
}

eastl::string8 TelemetryEvent::getMGS(MGS mgs) const
{
    eastl::string8 strMGS;
    uint32_t idx = getMetricID();
    if (0 == idx)
    {
        return strMGS;
    }
    char8_t** MetricListAccessor = (char8_t**)METRIC_LIST[idx] + mgs;
    if ((*MetricListAccessor) != NULL)
    {
        strMGS = (*MetricListAccessor);
    }

    return strMGS;
}

eastl::string8 TelemetryEvent::getModuleStr() const
{
    return getMGS(MODULE);
}

eastl::string8 TelemetryEvent::getGroupStr() const
{
    return getMGS(GROUP);
}

eastl::string8 TelemetryEvent::getStringStr() const
{
    return getMGS(STRING);
}

eastl::string8 TelemetryEvent::getParams() const
{
    eastl::string8 paramString = " ";

    char8_t* totalMem = m_eventBufferSharedPtr.get();

    TelemetryFileChunk *chunkHeadPtr = reinterpret_cast<TelemetryFileChunk*>(totalMem);
    char8_t* attributePtr = reinterpret_cast<char8_t*>( reinterpret_cast<char8_t*>(chunkHeadPtr) + sizeof(TelemetryFileChunk));

    // Skip Module Group String.
    char8_t** MetricListAccessor = (char8_t**)METRIC_LIST[getMetricID()] + SKIP_MGS;

    if ((*MetricListAccessor) == NULL)
        return paramString;

    for(uint32_t i=0; i<chunkHeadPtr->attributeCount; i++)
    {
        eastl::string8 intAsString;
        TelemetryFileAttributItem* value = (TelemetryFileAttributItem*)attributePtr;

        switch( ((TelemetryFileAttributItem*)attributePtr)->type ) 
        {
            case TYPEID_I8:
            {
                TelemetryDataFormatType tmp;
                GetAttribute(&(value->value), reinterpret_cast<char8_t*>(&tmp), value->valueSize);
                intAsString.append_sprintf("%i", tmp.i8);
                paramString += (*MetricListAccessor)==NULL ?"":*MetricListAccessor;
                paramString += "=\"" + intAsString + "\" ";
            }
            break;
            case TYPEID_I16:
            {
                TelemetryDataFormatType tmp;
                GetAttribute(&(value->value), reinterpret_cast<char8_t*>(&tmp), value->valueSize);
                intAsString.append_sprintf("%i", tmp.i16);
                paramString += (*MetricListAccessor)==NULL ?"":*MetricListAccessor;
                paramString += "=\"" + intAsString + "\" ";
            }
            break;

            case TYPEID_U16:
            {
                TelemetryDataFormatType tmp;
                GetAttribute(&(value->value), reinterpret_cast<char8_t*>(&tmp), value->valueSize);
                intAsString.append_sprintf("%u", tmp.u16);
                paramString += (*MetricListAccessor)==NULL ?"":*MetricListAccessor;
                paramString += "=\"" + intAsString + "\" ";
            }
            break;

            case TYPEID_I32:
            {
                TelemetryDataFormatType tmp;
                GetAttribute(&(value->value), reinterpret_cast<char8_t*>(&tmp), value->valueSize);
                intAsString.append_sprintf("%i", tmp.i32);
                paramString += (*MetricListAccessor)==NULL ?"":*MetricListAccessor;
                paramString += "=\"" + intAsString + "\" ";
            }
                break;

            case TYPEID_U32:
            {
                TelemetryDataFormatType tmp;
                GetAttribute(&(value->value), reinterpret_cast<char8_t*>(&tmp), value->valueSize);
                intAsString.append_sprintf("%lu", tmp.u32);
                paramString += (*MetricListAccessor)==NULL ?"":*MetricListAccessor;
                paramString += "=\"" + intAsString + "\" ";
            }
            break;

            case TYPEID_I64:
            {
                TelemetryDataFormatType tmp;
                GetAttribute(&(value->value), reinterpret_cast<char8_t*>(&tmp), value->valueSize);
                intAsString.append_sprintf("%I64i", tmp.i64);
                paramString += (*MetricListAccessor)==NULL ?"":*MetricListAccessor;
                paramString += "=\"" + intAsString + "\" ";
            }
            break;

            case TYPEID_U64:
            {
                TelemetryDataFormatType tmp;
                GetAttribute(&(value->value), reinterpret_cast<char8_t*>(&tmp), value->valueSize);
                intAsString.append_sprintf("%I64u", tmp.u64);
                paramString += (*MetricListAccessor)==NULL ?"":*MetricListAccessor;
                paramString += "=\"" + intAsString + "\" ";
            }
            break;

            case TYPEID_S8:
            {
                uint64_t s=value->valueSize+1;
                char8_t* tmp = (char8_t*)malloc((size_t)s);
                memset(tmp, 0, (size_t)s);
                GetAttribute(&(value->value), tmp, value->valueSize);

                // insert the escape sequences for '&', '>', '<' and '"'
                std::string tempString(tmp);
                NonQTOriginServices::Utilities::escape(tempString);

                intAsString.append_sprintf("%s", tempString.c_str());
                paramString += (*MetricListAccessor)==NULL ?"":*MetricListAccessor;
                paramString += "=\"" + intAsString + "\" ";
                free((void*)tmp);
            }
            break;

            case TYPEID_S16:
            {
                uint64_t s=value->valueSize+1;
                char8_t* tmp = (char8_t*)malloc((size_t)s);
                memset(tmp, 0, (size_t)s);
                GetAttribute(&(value->value), tmp, value->valueSize);

                // insert the escape sequences for '&', '>', '<' and '"'
                std::string tempString(tmp);
                NonQTOriginServices::Utilities::escape(tempString);

                intAsString.append_sprintf("%s", tempString.c_str());
                paramString += (*MetricListAccessor)==NULL ?"":*MetricListAccessor;
                paramString += "=\"" + intAsString + "\" ";
                free((void*)tmp);
            }
            break;

            case TYPEID_S32:
            {
                uint64_t s=value->valueSize+1;
                char8_t* tmp = (char8_t*)malloc((size_t)s);
                memset(tmp, 0, (size_t)s);
                GetAttribute(&(value->value), tmp, value->valueSize);
                intAsString.append_sprintf("%s", (char8_t*)tmp);
                paramString += (*MetricListAccessor)==NULL ?"":*MetricListAccessor;
                paramString += "=\"" + intAsString + "\" ";
                free((void*)tmp);
            }
            break;

            default:
                EA_FAIL_MSG("TelemetryEvent::getParams() unknown type in Attributes.");
            break;
        } // end - for

        if( NULL != *MetricListAccessor)
        {
            MetricListAccessor++;
            attributePtr+=((TelemetryFileAttributItem*)attributePtr)->valueSize;
            attributePtr+= sizeof(TelemetryFileAttributItem);
        }
        else
        {
            EA_FAIL_MSG("TelemetryEvent::getParams() MetricListAccessor is null before we've processed all attributes.");
            break;
        }
    }

    return paramString;
}

const char* TelemetryEvent::subscriberStatus() const 
{ 
    return TelemetrySubscriptionStrings::TELEMETRY_STATUS_STRINGS[mSubscriberStatus]; 
}

} //namespace OriginTelemetry