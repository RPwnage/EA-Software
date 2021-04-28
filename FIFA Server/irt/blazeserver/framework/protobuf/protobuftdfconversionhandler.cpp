/*************************************************************************************************/
/*!
    \file   protobuftdfconversionhandler.cpp

    \attention
        (c) Electronic Arts Inc. 2017
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/protobuf/protobuftdfconversionhandler.h"

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/port.h>
#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/any.pb.h>

// Some type support in generics such as maps and other messages are left unfinished as per internal discussion.
// This is because no use case have been found for such. We may want to address it at some point or wait until
// this becomes necessary. 

// We'd also want to add support for a generic to contain another generic or variable to contain another variable etc. 
namespace Blaze
{
namespace protobuf
{

bool ProtobufTdfConversionHandler::isTdfIdRegistered(EA::TDF::TdfId tdfId) const
{
    return (mRegistry.find(tdfId) != mRegistry.end());
}

bool ProtobufTdfConversionHandler::registerTdf(ProtobufTdfRegistration& tdfRegistration)
{
    BLAZE_TRACE(Log::SYSTEM, "ProtobufTdfConversionHandler.registerTdf: registering EA::TDF::Tdf(%s) with EA::TDF::TdfId(%u)", 
        tdfRegistration.getTdfName(), tdfRegistration.getTdfId());

    ProtobufTdfConversionMap::const_iterator itr = mRegistry.find(tdfRegistration.getTdfId());
    if (itr != mRegistry.end())
    {
        if (strcmp(tdfRegistration.getTdfName(), ((ProtobufTdfRegistration&)(*itr)).getTdfName()) != 0)
        {
            BLAZE_FATAL(Log::SYSTEM, "ProtobufTdfConversionHandler.registerTdf: got EA::TDF::TdfId conflict for "
                "EA::TDF::TdfId(%u) is already registered for %s so it cannot be used again for %s", 
                tdfRegistration.getTdfId(), ((ProtobufTdfRegistration&)(*itr)).getTdfName(), 
                tdfRegistration.getTdfName());
            return false;
        }
        else
        {
            //already registered
            return true;
        }
    }
    mRegistry.insert(tdfRegistration);
    return true;
}

void ProtobufTdfConversionHandler::deregisterTdf(EA::TDF::TdfId tdfId)
{
    ProtobufTdfConversionMap::iterator itr = mRegistry.find(tdfId);
    if (itr != mRegistry.end())
    {
        mRegistry.erase(tdfId);
    }
}

static eastl::string StringReplace(const eastl::string& s, const eastl::string& oldsub, const eastl::string& newsub, bool replaceAll)
{
    eastl::string res;

    if (oldsub.empty()) {
        res.append(s);  // if empty, append the given string.
        return res;
    }

    eastl::string::size_type start_pos = 0;
    eastl::string::size_type pos;
    do {
        pos = s.find(oldsub, start_pos);
        if (pos == eastl::string::npos) {
            break;
        }
        res.append(s, start_pos, pos - start_pos);
        res.append(newsub);
        start_pos = pos + oldsub.size();  // start searching again after the "old"
    } while (replaceAll);
    res.append(s, start_pos, s.length() - start_pos);

    return res;
}

static eastl::string TdfTypeNameFromProtoTypeUrl(const eastl::string& protoTypeUrl)
{
    eastl::string tdfTypeName;
    
    size_t urlEnd = protoTypeUrl.find_last_of("/");
    if (urlEnd != eastl::string::npos && ((urlEnd + 1) != protoTypeUrl.size()))
    {
        tdfTypeName = StringReplace(protoTypeUrl.substr(urlEnd + 1), ".", "::", true);
        if (tdfTypeName.find_first_of("eadp::blaze") == 0)
            tdfTypeName = StringReplace(tdfTypeName, "eadp::blaze", "Blaze", false);
    }
    
    return tdfTypeName;
}

static eastl::string ProtoTypeUrlFromTdfTypeName(const eastl::string& tdfTypeName)
{
    // The url part pointing to googleapis.com does not really matter as we don't use it. Kept it like that to be same as Protobuf lib. 
    eastl::string protoTypeUrl(eastl::string("type.googleapis.com/") + StringReplace(tdfTypeName, "::", ".", true));
    if (tdfTypeName.find_first_of("Blaze") == 0)
        protoTypeUrl = StringReplace(protoTypeUrl, "Blaze", "eadp.blaze", false);
    return protoTypeUrl;
}


void ProtobufTdfConversionHandler::convertAnyToVariable(const ::google::protobuf::Any& any, EA::TDF::VariableTdfBase& variable)
{
    eastl::string tdfFullName(TdfTypeNameFromProtoTypeUrl(eastl::string(any.type_url().c_str())));
    EA::TDF::TdfId tdfId = EA::TDF::TdfFactory::get().getTdfIdFromFullClassName(tdfFullName.c_str());

    ProtobufTdfConversionMap::const_iterator regItr = mRegistry.find(tdfId);
    if (regItr != mRegistry.end())
    {
        if (variable.create(tdfId))
        {
            google::protobuf::io::ArrayInputStream input_stream(any.value().data(), any.value().size());
            google::protobuf::io::CodedInputStream coded_input_stream(&input_stream);

            auto fromProtobuf = ((ProtobufTdfRegistration&)(*regItr)).getConvertFromProtobuf();
            fromProtobuf(variable.get(), &coded_input_stream);
        }
    }
}

void ProtobufTdfConversionHandler::convertVariableToAny(const EA::TDF::VariableTdfBase& variable, ::google::protobuf::Any& any)
{
    convertTdfToAny(variable.get(), any);
}

void ProtobufTdfConversionHandler::convertTdfToAny(const EA::TDF::Tdf* tdf, ::google::protobuf::Any& any)
{
    if (tdf == nullptr)
        return;

    ProtobufTdfConversionMap::const_iterator regItr = mRegistry.find(tdf->getTdfId());
    if (regItr != mRegistry.end())
    {
        auto toProtobuf = ((ProtobufTdfRegistration&)(*regItr)).getConvertToProtobuf();

        std::string outputStr;
        google::protobuf::io::StringOutputStream str_stream(&outputStr);
        google::protobuf::io::CodedOutputStream coded_out_stream(&str_stream);
        toProtobuf(tdf, &coded_out_stream);

        any.set_value(outputStr.data(), coded_out_stream.ByteCount());
        any.set_type_url(ProtoTypeUrlFromTdfTypeName(tdf->getTypeDescription().getFullName()).c_str());
    }
}

void ProtobufTdfConversionHandler::convertProtoGenericToTdfGeneric(const Blaze::protobuf::generic& protoGeneric, EA::TDF::GenericTdfType& tdfGeneric)
{
    switch (protoGeneric.u_case())
    {
    case generic::kFloat:
        tdfGeneric.get().set(protoGeneric.float_().value());
        break;

    case generic::kInt64:
        tdfGeneric.get().set(protoGeneric.int64().value());
        break;

    case generic::kUint64:
        tdfGeneric.get().set(protoGeneric.uint64().value());
        break;

    case generic::kInt32:
        tdfGeneric.get().set(protoGeneric.int32().value());
        break;

    case generic::kUint32:
        tdfGeneric.get().set(protoGeneric.uint32().value());
        break;

    case generic::kBool:
        tdfGeneric.get().set(protoGeneric.bool_().value());
        break;

    case generic::kString:
        tdfGeneric.get().set(protoGeneric.string().value().c_str());
        break;

    case generic::kBytes:
    {
        EA::TDF::TdfBlob blob;
        blob.setData((const uint8_t*)(protoGeneric.bytes().value().data()), protoGeneric.bytes().value().length());
        tdfGeneric.get().set(blob);

        // The following, which would be slightly more efficient, does not work because asBlob() can't be called without first setting the value to be the blob.
        //tdfGeneric.get().asBlob().setData((const uint8_t*)(protoGeneric.bytes().value().data()), protoGeneric.bytes().value().length());
        break;
    }

    case generic::kObjectType:
    {
        EA::TDF::ObjectType objectType;
        convertProtoObjectTypeToTdfObjectType(protoGeneric.objecttype(), objectType);
        tdfGeneric.get().set(objectType);
        break;
    }

    case generic::kObjectId:
    {
        EA::TDF::ObjectId objectId;
        convertProtoObjectIdToTdfObjectId(protoGeneric.objectid(), objectId);
        tdfGeneric.get().set(objectId);
        break;
    }

    case generic::kTimeValue:
    {
        EA::TDF::TimeValue timeValue;
        convertProtoTimeValueToTdfTimeValue(protoGeneric.timevalue(), timeValue);
        tdfGeneric.get().set(timeValue);
        break;
    }

    case generic::kAny:
    {
        eastl::string tdfFullName(TdfTypeNameFromProtoTypeUrl(eastl::string(protoGeneric.any().type_url().c_str())));
        if (protoGeneric.any().Is<Blaze::protobuf::GenericList>())
        {
            Blaze::protobuf::GenericList protoList;
            protoGeneric.any().UnpackTo(&protoList); // source

            convertProtoGenericListToTdfGeneric(protoList, tdfGeneric);
        }
        else
        {
            convertAnyToGeneric(protoGeneric.any(), tdfGeneric);
        }
       

    }
        break;

    case generic::U_NOT_SET:
        break;

    default:
        BLAZE_ERR_LOG(Log::SYSTEM, "Blaze::protobuf::convertProtoGenericToTdfGeneric: Unhandled case\n" << protoGeneric.DebugString().c_str()); 
        break;
    }
}

void ProtobufTdfConversionHandler::convertTdfGenericToProtoGeneric(const EA::TDF::GenericTdfType& tdfGeneric, Blaze::protobuf::generic& protoGeneric)
{
    switch (tdfGeneric.get().getType())
    {
        // The support for variable, generic, map in a generic will only be implemented if it becomes absolutely necessary.
    case EA::TDF::TdfType::TDF_ACTUAL_TYPE_VARIABLE:
        break;
    case EA::TDF::TdfType::TDF_ACTUAL_TYPE_GENERIC_TYPE:
        break;
    case EA::TDF::TdfType::TDF_ACTUAL_TYPE_MAP:
        break;

    case EA::TDF::TdfType::TDF_ACTUAL_TYPE_LIST:
    {
        auto& tdfList = tdfGeneric.get().asList(); // source
        Blaze::protobuf::GenericList protoList; // destination 

        for (size_t i = 0; i < tdfList.vectorSize(); ++i)
        {
            EA::TDF::TdfGenericReferenceConst tdfEntry;
            tdfList.getValueByIndex(i, tdfEntry);

            EA::TDF::GenericTdfType tdfGenericEntry;
            tdfGenericEntry.set(tdfEntry);

            // convert source entry to destination entry
            convertTdfGenericToProtoGeneric(tdfGenericEntry, *(protoList.add_list()));
        }

        protoGeneric.mutable_any()->PackFrom(protoList);
        break;
    }

    case EA::TDF::TdfType::TDF_ACTUAL_TYPE_FLOAT:
        protoGeneric.mutable_float_()->set_value(tdfGeneric.get().asFloat());
        break;

    case EA::TDF::TdfType::TDF_ACTUAL_TYPE_ENUM:
        break;

    case EA::TDF::TdfType::TDF_ACTUAL_TYPE_STRING:
        protoGeneric.mutable_string()->set_value(tdfGeneric.get().asString());
        break;

    case EA::TDF::TdfType::TDF_ACTUAL_TYPE_BITFIELD:
        convertGenericToAny(tdfGeneric, *(protoGeneric.mutable_any()));
        break;

    case EA::TDF::TdfType::TDF_ACTUAL_TYPE_BLOB:
        protoGeneric.mutable_bytes()->set_value(tdfGeneric.get().asBlob().getData(), tdfGeneric.get().asBlob().getCount());
        break;

    case EA::TDF::TdfType::TDF_ACTUAL_TYPE_UNION:
        convertGenericToAny(tdfGeneric, *(protoGeneric.mutable_any()));
        break;

    case EA::TDF::TdfType::TDF_ACTUAL_TYPE_TDF:
        convertGenericToAny(tdfGeneric, *(protoGeneric.mutable_any()));
        break;

    case EA::TDF::TdfType::TDF_ACTUAL_TYPE_OBJECT_TYPE:
    {
        convertTdfObjectTypeToProtoObjectType(tdfGeneric.get().asObjectType(), *(protoGeneric.mutable_objecttype()));
        break;
    }

    case EA::TDF::TdfType::TDF_ACTUAL_TYPE_OBJECT_ID:
    {
        convertTdfObjectIdToProtoObjectId(tdfGeneric.get().asObjectId(), *(protoGeneric.mutable_objectid()));
        break;
    }

    case EA::TDF::TdfType::TDF_ACTUAL_TYPE_TIMEVALUE:
    {
        convertTdfTimeValueToProtoTimeValue(tdfGeneric.get().asTimeValue(), *(protoGeneric.mutable_timevalue()));
        break;
    }

    case EA::TDF::TdfType::TDF_ACTUAL_TYPE_BOOL:
        protoGeneric.mutable_bool_()->set_value(tdfGeneric.get().asBool());
        break;


        /* mutable_int32 for the 3 cases is not a typo - protobuf does not have a smaller integer type.*/
    case EA::TDF::TdfType::TDF_ACTUAL_TYPE_INT8:
        protoGeneric.mutable_int32()->set_value(tdfGeneric.get().asInt8());
        break;
    case EA::TDF::TdfType::TDF_ACTUAL_TYPE_INT16:
        protoGeneric.mutable_int32()->set_value(tdfGeneric.get().asInt16());
        break;
    case EA::TDF::TdfType::TDF_ACTUAL_TYPE_INT32:
        protoGeneric.mutable_int32()->set_value(tdfGeneric.get().asInt32());
        break;



        /* mutable_uint32 for the 3 cases is not a typo - protobuf does not have a smaller unsigned type.*/
    case EA::TDF::TdfType::TDF_ACTUAL_TYPE_UINT8:
        protoGeneric.mutable_uint32()->set_value(tdfGeneric.get().asUInt8());
        break;
    case EA::TDF::TdfType::TDF_ACTUAL_TYPE_UINT16:
        protoGeneric.mutable_uint32()->set_value(tdfGeneric.get().asUInt16());
        break;
    case EA::TDF::TdfType::TDF_ACTUAL_TYPE_UINT32:
        protoGeneric.mutable_uint32()->set_value(tdfGeneric.get().asUInt32());
        break;



    case EA::TDF::TdfType::TDF_ACTUAL_TYPE_INT64:
        protoGeneric.mutable_int64()->set_value(tdfGeneric.get().asInt64());
        break;

    case EA::TDF::TdfType::TDF_ACTUAL_TYPE_UINT64:
        protoGeneric.mutable_uint64()->set_value(tdfGeneric.get().asUInt64());
        break;

    default:
        break;
    }
}

void ProtobufTdfConversionHandler::convertAnyToGeneric(const ::google::protobuf::Any& any, EA::TDF::GenericTdfType& generic)
{
    eastl::string tdfFullName(TdfTypeNameFromProtoTypeUrl(eastl::string(any.type_url().c_str())));
    EA::TDF::TdfId tdfId = EA::TDF::TdfFactory::get().getTdfIdFromFullClassName(tdfFullName.c_str());

    ProtobufTdfConversionMap::const_iterator regItr = mRegistry.find(tdfId);
    if (regItr != mRegistry.end())
    {
        EA::TDF::TdfBitfield* bitfield = EA::TDF::TdfFactory::get().createBitField(tdfId);
        if (bitfield)
        {
            google::protobuf::io::ArrayInputStream input_stream(any.value().data(), any.value().size());
            google::protobuf::io::CodedInputStream coded_input_stream(&input_stream);

            auto fromProtobuf = ((ProtobufTdfRegistration&)(*regItr)).getConvertFromProtobuf();
            fromProtobuf(bitfield, &coded_input_stream);

            generic.get().set(*bitfield);
            delete bitfield;
        }
        else //Should support union and a message. 
        {
            //tdf->getTdfType() == EA::TDF::TdfType::TDF_ACTUAL_TYPE_UNION
            EA::TDF::Tdf* tdf = EA::TDF::TdfFactory::get().create(tdfId);
            if (tdf)
            {
                google::protobuf::io::ArrayInputStream input_stream(any.value().data(), any.value().size());
                google::protobuf::io::CodedInputStream coded_input_stream(&input_stream);

                auto fromProtobuf = ((ProtobufTdfRegistration&)(*regItr)).getConvertFromProtobuf();
                fromProtobuf(tdf, &coded_input_stream);

                generic.get().set(*tdf);
                delete tdf;
            }
        }
    }

}

void ProtobufTdfConversionHandler::convertGenericToAny(const EA::TDF::GenericTdfType& generic, ::google::protobuf::Any& any)
{
    EA::TDF::TdfId tdfId = generic.get().getTypeDescription().getTdfId();

    ProtobufTdfConversionMap::const_iterator regItr = mRegistry.find(tdfId);
    if (regItr != mRegistry.end())
    {
        auto toProtobuf = ((ProtobufTdfRegistration&)(*regItr)).getConvertToProtobuf();

        std::string outputStr;
        google::protobuf::io::StringOutputStream str_stream(&outputStr);
        google::protobuf::io::CodedOutputStream coded_out_stream(&str_stream);
        toProtobuf((void*)(generic.get().asAny()), &coded_out_stream);

        any.set_value(outputStr.data(), coded_out_stream.ByteCount());
        any.set_type_url(ProtoTypeUrlFromTdfTypeName(generic.get().getTypeDescription().getFullName()).c_str());
    }
}

void ProtobufTdfConversionHandler::convertProtoGenericListToTdfGenericInternal(const char8_t* tdfType, const Blaze::protobuf::GenericList& protoList, EA::TDF::GenericTdfType& tdfGeneric)
{
    EA::TDF::TdfGenericValue tdfGenericValue;
    
    if (EA::TDF::TdfFactory::get().createGenericValue(tdfType, tdfGenericValue))
    {
        for (int i = 0; i < protoList.list_size(); ++i)
        {
            EA::TDF::TdfGenericReference entry;
            tdfGenericValue.asList().pullBackRef(entry);
            
            switch (entry.getType())
            {
            case EA::TDF::TdfType::TDF_ACTUAL_TYPE_FLOAT:
            {
                entry.asFloat() = protoList.list(i).float_().value(); 
                break;
            }
            
            case  EA::TDF::TdfType::TDF_ACTUAL_TYPE_INT64:
            {
                entry.asInt64() = protoList.list(i).int64().value();
                break;
            }
            
            case  EA::TDF::TdfType::TDF_ACTUAL_TYPE_UINT64:
            {
                entry.asUInt64() = protoList.list(i).uint64().value();
                break;
            }
            
            case  EA::TDF::TdfType::TDF_ACTUAL_TYPE_INT32:
            {
                entry.asInt32() = protoList.list(i).int32().value();
                break;
            }
            
            case  EA::TDF::TdfType::TDF_ACTUAL_TYPE_UINT32:
            {
                entry.asUInt32() = protoList.list(i).uint32().value();
                break;
            }

            case  EA::TDF::TdfType::TDF_ACTUAL_TYPE_BOOL:
            {
                entry.asBool() = protoList.list(i).bool_().value();
                break;
            }

            case  EA::TDF::TdfType::TDF_ACTUAL_TYPE_STRING:
            {
                entry.asString() = protoList.list(i).string().value().c_str();
                break;
            }

            case  EA::TDF::TdfType::TDF_ACTUAL_TYPE_OBJECT_TYPE:
            {
                convertProtoObjectTypeToTdfObjectType(protoList.list(i).objecttype(), entry.asObjectType());
                break;
            }

            case  EA::TDF::TdfType::TDF_ACTUAL_TYPE_OBJECT_ID:
            {
                convertProtoObjectIdToTdfObjectId(protoList.list(i).objectid(), entry.asObjectId());
                break;
            }

            case  EA::TDF::TdfType::TDF_ACTUAL_TYPE_TIMEVALUE:
            {
                convertProtoTimeValueToTdfTimeValue(protoList.list(i).timevalue(), entry.asTimeValue());
                break;
            }

            default:
                break;
            }
        }
    }

    tdfGeneric.get().set(tdfGenericValue);
}

void ProtobufTdfConversionHandler::convertProtoGenericListToTdfGeneric(const Blaze::protobuf::GenericList& protoList, EA::TDF::GenericTdfType& tdfGeneric)
{
    if (protoList.list_size() == 0)
        return;

    auto& firstElement = protoList.list(0);

    switch (firstElement.u_case())
    {
    case generic::kFloat:
    {
        convertProtoGenericListToTdfGenericInternal("list<float>", protoList, tdfGeneric);
        break;
    }

    case generic::kInt64:
    {
        convertProtoGenericListToTdfGenericInternal("list<int64_t>", protoList, tdfGeneric);
        break;
    }

    case generic::kUint64:
    {
        convertProtoGenericListToTdfGenericInternal("list<uint64_t>", protoList, tdfGeneric);
        break;
    }

    case generic::kInt32:
    {
        convertProtoGenericListToTdfGenericInternal("list<int32_t>", protoList, tdfGeneric);
        break;
    }

    case generic::kUint32:
    {
        convertProtoGenericListToTdfGenericInternal("list<uint32_t>", protoList, tdfGeneric);
        break;
    }

    case generic::kBool:
    {
        convertProtoGenericListToTdfGenericInternal("list<bool>", protoList, tdfGeneric);
        break;
    }

    case generic::kString:
    {
        convertProtoGenericListToTdfGenericInternal("list<string>", protoList, tdfGeneric);
        break;
    }

    case generic::kBytes:
    {
        break;
    }

    case generic::kObjectType:
    {
        convertProtoGenericListToTdfGenericInternal("list<ObjectType>", protoList, tdfGeneric);
        break;
    }

    case generic::kObjectId:
    {
        convertProtoGenericListToTdfGenericInternal("list<ObjectId>", protoList, tdfGeneric);
        break;
    }

    case generic::kTimeValue:
    {
        convertProtoGenericListToTdfGenericInternal("list<TimeValue>", protoList, tdfGeneric);
        break;
    }

    case generic::kAny:
    {
        break;
    }
    default:
        break;
    }

}

void ProtobufTdfConversionHandler::convertProtoObjectTypeToTdfObjectType(const Blaze::protobuf::ObjectType& protoObjectType, EA::TDF::ObjectType& tdfObjectType)
{
    tdfObjectType.component = static_cast<EA::TDF::ComponentId>(protoObjectType.component());
    tdfObjectType.type = static_cast<EA::TDF::EntityType>(protoObjectType.type());
}

void ProtobufTdfConversionHandler::convertTdfObjectTypeToProtoObjectType(const EA::TDF::ObjectType& tdfObjectType, Blaze::protobuf::ObjectType& protoObjectType)
{
    protoObjectType.set_component(tdfObjectType.component);
    protoObjectType.set_type(tdfObjectType.type);
}

void ProtobufTdfConversionHandler::convertProtoObjectIdToTdfObjectId(const Blaze::protobuf::ObjectId& protoObjectId, EA::TDF::ObjectId& tdfObjectId)
{
    tdfObjectId.type.component = static_cast<EA::TDF::ComponentId>(protoObjectId.type().component());
    tdfObjectId.type.type = static_cast<EA::TDF::EntityType>(protoObjectId.type().type());
    tdfObjectId.id = protoObjectId.id();
}

void ProtobufTdfConversionHandler::convertTdfObjectIdToProtoObjectId(const EA::TDF::ObjectId& tdfObjectId, Blaze::protobuf::ObjectId& protoObjectId)
{
    protoObjectId.mutable_type()->set_component(tdfObjectId.type.component);
    protoObjectId.mutable_type()->set_type(tdfObjectId.type.type);
    protoObjectId.set_id(tdfObjectId.id);
}

void ProtobufTdfConversionHandler::convertProtoTimeValueToTdfTimeValue(const Blaze::protobuf::TimeValue& protoTimeValue, EA::TDF::TimeValue& tdfTimeValue)
{
    tdfTimeValue.setMicroSeconds(protoTimeValue.microseconds());
}

void ProtobufTdfConversionHandler::convertTdfTimeValueToProtoTimeValue(const EA::TDF::TimeValue& tdfTimeValue, Blaze::protobuf::TimeValue& protoTimeValue)
{
    protoTimeValue.set_microseconds(tdfTimeValue.getMicroSeconds());
}

ProtobufTdfConversionHandler::~ProtobufTdfConversionHandler()
{

}

ProtobufTdfRegistration::ProtobufTdfRegistration(const char8_t* fullclassname, TdfConvertToProtobuf convertToProtobuf, TdfConvertFromProtobuf convertFromProtobuf, EA::TDF::TdfId tdfId):
    mTdfId(tdfId),
    mTdfToProtobuf(convertToProtobuf),
    mTdfFromProtobuf(convertFromProtobuf)
{
    bool success = true;
    
    if(strlen(fullclassname) >= EA::TDF::MAX_TDF_NAME_LEN)
    {
        BLAZE_WARN(Log::SYSTEM, "ProtobufTdfRegistration.ProtobufTdfRegistration(): fullclassname(%s) will be truncated since buffer size is only %u",
                fullclassname, (uint32_t)EA::TDF::MAX_TDF_NAME_LEN);        
    }

    blaze_strnzcpy(mTdfName, fullclassname, EA::TDF::MAX_TDF_NAME_LEN);

    success = ProtobufTdfConversionHandler::get().registerTdf(*this);

    EA_ASSERT(success);
}

ProtobufTdfRegistration::~ProtobufTdfRegistration()
{
    ProtobufTdfConversionHandler::get().deregisterTdf(mTdfId);
}

}
}


