///////////////////////////////////////////////////////////////////////////////
// BsonDomReader.h
// 
// Copyright (c) 2011 Electronic Arts Inc.
// Written and maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////


#ifndef EAJSON_BSONDOMREADER_H
#define EAJSON_BSONDOMREADER_H


#include <EAJson/internal/Config.h>
#include <EAJson/JsonDomReader.h>
#include <EAJson/BsonReader.h>
#include <coreallocator/icoreallocator_interface.h>


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251) // 'x' needs to have dll-interface to be used by clients of class 'y'.
#endif


namespace EA
{
    namespace Json
    {
            
        //////////////////////////////////////////////////////////////////////////
        /// A DOM building reader for binary JSON (BSON)
        ///
        /// Reads a BSON stream into a JsonDomDocument. 
        /// A JsonDomDocument is independent of the JSON stream format, as it is
        /// a tree in memory.
        //////////////////////////////////////////////////////////////////////////
        
        class EAJSON_API BsonDomReader : public BsonReader
        {
        public:
            /// Default constructor
            /// The pAllocator is the allocator used for the BsonReader, which may
            /// be different from the allocator the user chooses to associated with
            /// the DOM that is build with the Build function.
            BsonDomReader(EA::Allocator::ICoreAllocator* pAllocator = NULL, size_t bufferBlockSize = 0);
            
            /// Build the DOM. 
            /// This function builds the Dom with domNode's allocator. If domNode 
            /// has no allocator then it builds the Dom with this class' allocator.
            /// It is expected that you have called BsonReader base functions to 
            /// set up the class instance before doing this parse.
            /// The domNode parameter may be any type of JsonDomNode, but typically the
            /// user would provide a JsonDomDocument node.
            /// The DOM is built into the domNode with the ICoreAllocator that is 
            /// associated with the domNode. If domNode's allocator is NULL then
            /// the allocator of this BsonDomReader class will be used.
            /// Upon error the DOM is left in its built state upon encountering the error.
            /// The JSON input data must be set via SetStream or SetString.
            Result Build(JsonDomNode& domNode);
            
        protected:
            void AddChildNode(JsonDomNode* pParentNode, JsonDomNode* pChildNode);
        };

    } // namespace Json
    
} // namespace EA



#ifdef _MSC_VER
#pragma warning(pop)
#endif


#endif // Header include guard
