/*************************************************************************************************/
/*!
    \file   protobuftdfconversionhandler.h

    \attention
        (c) Electronic Arts Inc. 2017
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ProtobufTdfConversionHandler

    Handles scenarios where explicit conversion between Protobuf and TDF objects is required. This is required 
    because of syntactical and other code generation differences in some scenarios. 
*/
/*************************************************************************************************/
#ifndef PROTOBUF_TDF_CONVERSION_HANDLER_H
#define PROTOBUF_TDF_CONVERSION_HANDLER_H

#include "EATDF/tdf.h"

#include "EASTL/hash_map.h"
#include "EASTL/intrusive_ptr.h"
#include "EASTL/intrusive_hash_map.h"

#include <functional>
#include "eadp/blaze/protobuf/blazewellknownmessages.pb.h"


namespace google
{
namespace protobuf
{
    class Any;
namespace io
{
    class CodedInputStream;
    class CodedOutputStream;
}
}
}

namespace Blaze
{
namespace protobuf
{

using generic = eadp::blaze::protobuf::generic;
using ObjectType = eadp::blaze::protobuf::ObjectType;
using ObjectId = eadp::blaze::protobuf::ObjectId;
using TimeValue = eadp::blaze::protobuf::TimeValue;
using GenericList = eadp::blaze::protobuf::GenericList;

class ProtobufTdfRegistration;

using TdfConvertToProtobuf = std::function<void(const void*, google::protobuf::io::CodedOutputStream* output)>;
using TdfConvertFromProtobuf = std::function<bool(void*, google::protobuf::io::CodedInputStream* input)>;

struct ProtobufTdfConversionMapNode : public eastl::intrusive_hash_node {};
typedef eastl::intrusive_hash_map<EA::TDF::TdfId, ProtobufTdfConversionMapNode, TDF_FACTORY_BUCKET_COUNT> ProtobufTdfConversionMap;

class ProtobufTdfConversionHandler
{
public:
    static ProtobufTdfConversionHandler& get() { static ProtobufTdfConversionHandler instance; return instance; }
    
    bool isTdfIdRegistered(EA::TDF::TdfId tdfId) const;

    bool registerTdf(ProtobufTdfRegistration& tdfRegistration);
    void deregisterTdf(EA::TDF::TdfId tdfId);

    void convertAnyToVariable(const ::google::protobuf::Any& any, EA::TDF::VariableTdfBase& variable);
    void convertVariableToAny(const EA::TDF::VariableTdfBase& variable, ::google::protobuf::Any& any);
    void convertTdfToAny(const EA::TDF::Tdf* tdf, ::google::protobuf::Any& any);

    void convertProtoGenericToTdfGeneric(const Blaze::protobuf::generic& protoGeneric, EA::TDF::GenericTdfType& tdfGeneric);
    void convertTdfGenericToProtoGeneric(const EA::TDF::GenericTdfType& tdfGeneric, Blaze::protobuf::generic& protoGeneric);
    
    void convertProtoObjectTypeToTdfObjectType(const Blaze::protobuf::ObjectType& protoObjectType, EA::TDF::ObjectType& tdfObjectType);
    void convertTdfObjectTypeToProtoObjectType(const EA::TDF::ObjectType& tdfObjectType, Blaze::protobuf::ObjectType& protoObjectType);

    void convertProtoObjectIdToTdfObjectId(const Blaze::protobuf::ObjectId& protoObjectId, EA::TDF::ObjectId& tdfObjectId);
    void convertTdfObjectIdToProtoObjectId(const EA::TDF::ObjectId& tdfObjectId, Blaze::protobuf::ObjectId& protoObjectId);

    void convertProtoTimeValueToTdfTimeValue(const Blaze::protobuf::TimeValue& protoTimeValue, EA::TDF::TimeValue& tdfTimeValue);
    void convertTdfTimeValueToProtoTimeValue(const EA::TDF::TimeValue& tdfTimeValue, Blaze::protobuf::TimeValue& protoTimeValue);

    
private:
   ~ProtobufTdfConversionHandler();
    ProtobufTdfConversionMap mRegistry;

    void convertAnyToGeneric(const ::google::protobuf::Any& any, EA::TDF::GenericTdfType& generic);
    void convertGenericToAny(const EA::TDF::GenericTdfType& generic, ::google::protobuf::Any& any);

    void convertProtoGenericListToTdfGeneric(const Blaze::protobuf::GenericList& protoList, EA::TDF::GenericTdfType& tdfGeneric);
    void convertProtoGenericListToTdfGenericInternal(const char8_t* tdfType, const Blaze::protobuf::GenericList& protoList, EA::TDF::GenericTdfType& tdfGeneric);

};

class ProtobufTdfRegistration : 
    private ProtobufTdfConversionMapNode
{
    friend class ProtobufTdfConversionHandler;
public:
    ProtobufTdfRegistration(const char8_t* fullclassname, TdfConvertToProtobuf convertToProtobuf, TdfConvertFromProtobuf convertFromProtobuf, EA::TDF::TdfId tdfId);
   ~ProtobufTdfRegistration();

    EA::TDF::TdfId getTdfId() const { return mTdfId; }
    const char8_t* getTdfName() const { return mTdfName; }
    const TdfConvertToProtobuf getConvertToProtobuf() { return mTdfToProtobuf; }
    const TdfConvertFromProtobuf getConvertFromProtobuf() { return mTdfFromProtobuf; }

private:
    friend struct eastl::use_intrusive_key<ProtobufTdfConversionMapNode, EA::TDF::TdfId>;

    EA::TDF::TdfId mTdfId;
    char8_t mTdfName[EA::TDF::MAX_TDF_NAME_LEN];
    TdfConvertToProtobuf mTdfToProtobuf;
    TdfConvertFromProtobuf mTdfFromProtobuf;
};


} // namespace protobuf
} // namespace Blaze

namespace eastl
{
    template <>
    struct use_intrusive_key<Blaze::protobuf::ProtobufTdfConversionMapNode, EA::TDF::TdfId>
    {
        EA::TDF::TdfId operator()(const Blaze::protobuf::ProtobufTdfConversionMapNode& x) const
        { 
            return static_cast<const Blaze::protobuf::ProtobufTdfRegistration &>(x).getTdfId();
        }
    };
}

#endif // PROTOBUF_TDF_CONVERSION_HANDLER_H
