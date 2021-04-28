///////////////////////////////////////////////////////////////////////////////
// JsonDomReader.h
// 
// Copyright (c) 2009 Electronic Arts Inc.
// Written and maintained by Paul Pedriana.
//
// See http://www.json.org/ and http://www.ietf.org/rfc/rfc4627.txt
///////////////////////////////////////////////////////////////////////////////


#ifndef EAJSON_JSONDOMREADER_H
#define EAJSON_JSONDOMREADER_H


#include <EAJson/internal/Config.h>
#include <EAJson/JsonReader.h>
#include <EASTL/vector.h>
#include <EASTL/string.h>

#if defined(EA_PLATFORM_MICROSOFT) && defined(GetObject) // If a Microsoft header #defined GetObject to GetObjectA or GetObjectW...
    #undef GetObject
#endif

#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4251) // 'x' needs to have dll-interface to be used by clients of class 'y'.
#endif


namespace EA
{
    namespace Json
    {
        // Forward declarations
        class JsonDomNode;
        class JsonDomDocument;
        class JsonDomObjectValue;
        class JsonDomObject;
        class JsonDomArray;
        class JsonDomInteger;
        class JsonDomDouble;
        class JsonDomBool;
        class JsonDomString;
        class JsonDomNull;
        class JsonDomReader;
        class IJsonDomReaderCallback;


        // Typedefs
        typedef eastl::vector<JsonDomNode*,          EASTLCoreAllocator> JsonDomNodeArray;
        typedef eastl::vector<JsonDomObjectValue,    EASTLCoreAllocator> JsonDomObjectValueArray;


        //////////////////////////////////////////////////////////////////////////
        /// Base node type for the DOM tree
        ///
        /// You must supply a valid non-null allocator when doing anything but 
        /// default constructing an instance of this class.
        /// To consider: Add child and sibling node finding functionality.
        //////////////////////////////////////////////////////////////////////////
        class EAJSON_API JsonDomNode
        {
        public:
            JsonDomNode(EventType nodeType = kETNone, EA::Allocator::ICoreAllocator* pAllocator = NULL);
            JsonDomNode(NoInit);
            JsonDomNode(const JsonDomNode&);
            JsonDomNode& operator=(const JsonDomNode&);

            virtual ~JsonDomNode();

            EA::Allocator::ICoreAllocator* GetAllocator() const;
            void SetAllocator(EA::Allocator::ICoreAllocator* pAllocator);

            /// The returned node is created with the member allocator, which must
            /// be valid.
            /// It might be useful for the user to subclass this function in order to 
            /// allow for extending the returned node type for some user purpose.
            virtual JsonDomNode* CreateNode(EventType nodeType) const;

            /// A destroy operation is the same thing as calling the object's 
            /// destructor and freeing its memory via the allocator associated
            /// with it. It is essentially the same thing as a custom member 
            /// operator delete. To consider: operator delete may be sufficient.
            void Destroy();

            /// A clone operation is the same thing as creating a new object and 
            /// copy-constructing it from this object. This is not the same thing
            /// as a custom member operator new because this function can be used
            /// to clone a subclass via a base class pointer.
            virtual JsonDomNode* Clone() const;

            /// Clears the contents of the node, leaving the node with a state of
            /// having no child nodes (if an array or object) or a zero value (if a
            /// basic type). The contents will be recursively cleared and destroyed. 
            /// No nodes that may be above this node will be affected.
            virtual void Clear();

            /// Iterate the contents of this node to the callback handler.
            /// This is also known as Accept in terms of the hierarchical 
            /// visitor pattern.
            ///
            /// If you return true from a IJsonDomReaderCallback function, recursive 
            /// parsing will continue. If you return false, no children of the 
            /// current node or its sibilings will be iterated.
            ///
            /// The pDomReaderCallback argument must be non-NULL.
            /// The user-specified IJsonDomReaderCallback -may- modify the child nodes
            /// of the currently iterated node in any way. The handler may modify
            /// the sibling and parent nodes of the currently iterated node only
            /// in ways that don't modify the tree structure.
            //////////////////////////////////////////////////////////////////////////
            virtual bool Iterate(IJsonDomReaderCallback* pDomReaderCallback) = 0;

            /// Returns a JsonDomNode given a string path to the node.
            /// Returns NULL if there is no matching node.
            /// You can downcast the resulting JsonDomNode with AsJsonDomObject, AsJsonDomArray, etc.
            /// or you can use the more specific Get functions such as GetObject, GetArray, etc.
            ///
            /// Given the following JSON:
            ///    [
            ///        "cow",
            ///        {
            ///             "hey" : 123, 
            ///             "abc" : [ "red", "blue", { "test" : "result",  "no" : "way" } ] 
            ///        }
            ///    ]
            /// 
            /// The following return values are:
            ///      | path             | result                   | comments     
            ///      ----------------------------------------------------------------------------
            ///       "/"                 JsonDomArray               Get top-level array itself.
            ///       "/0"                JsonDomString "cow"        Get the 0th array element.
            ///       "/1/hey"            JsonDomInteger 123         
            ///       "/1/hey/"           NULL                       The trailing / would work only if hey was a 'directory' (array or object).        
            ///       "/1//"              NULL                       Cannot have an empty path element.   
            ///       "/1/abc/2/test"     JsonDomString "result"     
            ///       "/1/abc/2/#0"       JsonDomString "result"     Using an object index instead of object name. Note that for objects (unlike array) you must prefix the index with a # so as to not collide with object names that may be numerical.
            ///       "/1/abc/2/blah"     NULL                       blah is a non-existant name.
            ///       "/1/abc/2/#37"      NULL                       37 is an out-of-range index.
            ///       "/37/cow"           NULL                       37 is an out-of-range index.
            ///       "/1"                JsonDomObject              Refers to the { } object after cow.              
            ///       "/1/"               JsonDomObject              Like disk file paths, we allow trailing separators on 'directories' (arrays or objects).              
            ///      ----------------------------------------------------------------------------
            JsonDomNode*       GetNode(const char8_t* pPath);
            const JsonDomNode* GetNode(const char8_t* pPath) const;

            /// These functions are more specific versions of GetNode.
            /// They return NULL if the node is not found or if the node type doesn't match 
            /// the function type (e.g. you use GetInteger but the node is actually a Double).
            JsonDomDocument*    GetDocument(const char8_t* pPath);
            JsonDomObject*      GetObject(const char8_t* pPath);
            JsonDomArray*       GetArray(const char8_t* pPath);
            JsonDomInteger*     GetInteger(const char8_t* pPath);
            JsonDomDouble*      GetDouble(const char8_t* pPath);
            JsonDomBool*        GetBool(const char8_t* pPath);
            JsonDomString*      GetString(const char8_t* pPath);
            JsonDomNull*        GetNull(const char8_t* pPath);

            /// Returns the parent node of this node. Returns NULL if the node
            /// has no parent.
            /// The parent node will always be an object node, an array node, or 
            /// document node (note that as of this writing JsonDomDocument is a 
            /// subclass of JsonDomArray.
            JsonDomNode* GetParent();

            /// Get the top-most node in the hierarchy to which this node belongs.
            /// This function essentially walks the mpParent values upward.
            /// If the current node has no parent, the current node is returned.
            /// This function always returns a valid JsonDomNode.
            JsonDomNode* GetTop();

            /// Accessors
            /// These functions expect that the node type matches the function and don't do
            /// type checking. They are meant to be very fast downcasting functions. You can
            /// use the GetNodeType function to test the node type before using these functions if necessary.
            /// These functions can't be used to test the node type; use GetNodeType for that.
            JsonDomDocument*    AsJsonDomDocument();
            JsonDomObject*      AsJsonDomObject();
            JsonDomArray*       AsJsonDomArray();
            JsonDomInteger*     AsJsonDomInteger();
            JsonDomDouble*      AsJsonDomDouble();
            JsonDomBool*        AsJsonDomBool();
            JsonDomString*      AsJsonDomString();
            JsonDomNull*        AsJsonDomNull();

            /// Returns the node type. You can safely downcast a JsonDomNode to a more specific
            /// node type based on the return value of this function.
            EventType GetNodeType() const;

        protected:
            friend class JsonDomReader;

            bool CopyChildList(const JsonDomNode& node);

        public:
            EventType       mNodeType;  // This can be any EventType except kETEnd*.
            String8         mName;      // Used to store a text representation of a node.
            JsonDomNode*    mpParent;   // This pointer is neither allocated nor owned by this class (children don't own parents; parents own children). 
        };


        // JsonDomObjectValue represents a JsonDomObject name:value pair. 
        // In the following JSON document, a JsonDomObjectValue is used to 
        // represent the "cost":23.99 part:
        //     { "cost":23.99, 123, 456 }
        // JsonDomObjectValue isn't a first class value in the JsonDomNode hierarcy. 
        // Rather JsonDomObjectValue is a pair stored in a container owned by JsonDomObject.

        class EAJSON_API JsonDomObjectValue
        {
        public:
            JsonDomObjectValue(EA::Allocator::ICoreAllocator* = NULL);
            JsonDomObjectValue(const JsonDomObjectValue&);
            JsonDomObjectValue& operator=(const JsonDomObjectValue&);

           ~JsonDomObjectValue();

            void Clear();

            // This following don't exist because JsonDomObjectValue isn't a first class value in the 
            // JsonDomNode hierarcy. Rather JsonDomObjectValue is a pair stored in a container owned
            // by JsonDomObject.
            //JsonDomNode* Clone() const;
            //bool Iterate(IJsonDomReaderCallback*);

            String8      mNodeName;
            JsonDomNode* mpNode;
        };


        // JsonDomObject represents a JSON object, which is a collection of zero or more 
        // named entities. It equates to the contents of a JSON document within {} characters.
        // The individual name/value pairs are represented by JsonDomObjectValue.

        class EAJSON_API JsonDomObject : public JsonDomNode
        {
        public:
            JsonDomObject(EA::Allocator::ICoreAllocator* = NULL);
            JsonDomObject(const JsonDomObject&);
            JsonDomObject& operator=(const JsonDomObject&);

           ~JsonDomObject();

            void         Clear();
            JsonDomNode* Clone() const;
            bool         CopyNodeArray(JsonDomObject* pDest) const;
            bool         Iterate(IJsonDomReaderCallback*);

            // This is a find function, for convenience. Other functionality such as insert and remove 
            // functionality should be done via manipulating mJsonDomNodeArray directly.
            JsonDomObjectValueArray::iterator GetNodeIterator(const char8_t* pName, bool bCaseSensitive);

            JsonDomObjectValueArray mJsonDomObjectValueArray;    // This is an array, but it's really like an unsorted STL multi-map. The primary difference is that this container needs to be unordered.
        };


        // JsonDomArray represents a JSON array. It equates to the contents of 
        // a JSON document within [] characters.
        class EAJSON_API JsonDomArray : public JsonDomNode
        {
        public:
            JsonDomArray(EA::Allocator::ICoreAllocator* = NULL);
            JsonDomArray(const JsonDomArray&);
            JsonDomArray& operator=(const JsonDomArray&);

           ~JsonDomArray();

            void         Clear();
            JsonDomNode* Clone() const;
            bool         CopyNodeArray(JsonDomArray* pDest) const;
            bool         Iterate(IJsonDomReaderCallback*);

            JsonDomNodeArray mJsonDomNodeArray;
        };


        //////////////////////////////////////////////////////////////////////////
        /// Document node
        ///
        /// The document node is a node that represents the document itself.
        /// The node name is the file name or path associated with the document,
        /// though it can be any moniker that identifies the DOM as a whole.
        ///
        /// Note that a document is an array (inherits from JsonDomArray). Thus we 
        /// support the concept that the top level of a JSON document can be a list 
        /// of 0 or more entities.
        ///
        /// To consider: add functionality to store information about how to 
        /// write this document to disk (e.g. tab style). Alternatively this 
        /// can be done at a higher level by the JsonDomWriter.
        //////////////////////////////////////////////////////////////////////////
        class EAJSON_API JsonDomDocument : public JsonDomArray
        {
        public:
            JsonDomDocument(EA::Allocator::ICoreAllocator* = NULL);
            JsonDomDocument(const JsonDomDocument&);
            JsonDomDocument& operator=(const JsonDomDocument&);

          //void         Clear();                           // Already exists in the parent JsonDomArray class.
          //JsonDomNode* Clone() const;                     // This is not necessary.
            bool         Iterate(IJsonDomReaderCallback*);
        };


        class EAJSON_API JsonDomInteger : public JsonDomNode
        {
        public:
            JsonDomInteger(EA::Allocator::ICoreAllocator* = NULL);
            JsonDomInteger(const JsonDomInteger&);
            JsonDomInteger& operator=(const JsonDomInteger&);

            JsonDomNode* Clone() const;
            bool         Iterate(IJsonDomReaderCallback*);

            int64_t mValue;
        };


        class EAJSON_API JsonDomDouble : public JsonDomNode
        {
        public:
            JsonDomDouble(EA::Allocator::ICoreAllocator* = NULL);
            JsonDomDouble(const JsonDomDouble&);
            JsonDomDouble& operator=(const JsonDomDouble&);

            JsonDomNode* Clone() const;
            bool         Iterate(IJsonDomReaderCallback*);

            double mValue;
        };


        class EAJSON_API JsonDomBool : public JsonDomNode
        {
        public:
            JsonDomBool(EA::Allocator::ICoreAllocator* = NULL);
            JsonDomBool(const JsonDomBool&);
            JsonDomBool& operator=(const JsonDomBool&);

            JsonDomNode* Clone() const;
            bool         Iterate(IJsonDomReaderCallback*);

            bool mValue;
        };


        class EAJSON_API JsonDomString : public JsonDomNode
        {
        public:
            JsonDomString(EA::Allocator::ICoreAllocator* = NULL);
            JsonDomString(const JsonDomString&);
            JsonDomString& operator=(const JsonDomString&);

            void         Clear();
            JsonDomNode* Clone() const;
            bool         Iterate(IJsonDomReaderCallback*);

            String8 mValue;
        };


        class EAJSON_API JsonDomNull : public JsonDomNode
        {
        public:
            JsonDomNull(EA::Allocator::ICoreAllocator* = NULL);
            JsonDomNull(const JsonDomNull&);
            JsonDomNull& operator=(const JsonDomNull&);

          //JsonDomNode* Clone() const; // This is not necessary.
            bool         Iterate(IJsonDomReaderCallback*);
        };



        //////////////////////////////////////////////////////////////////////////
        /// A DOM building reader
        ///
        /// We implement a DOM (Document Object Model) reader via inheriting from
        /// an iterative JsonReader and via EASTL containers. Since the tree is 
        /// implemented with EASTL containers, most of the functionality needed
        /// to iterate and manipulate the nodes and their contents can be done
        /// via standard STL functionality. 
        ///
        /// The JSON Standard (http://www.ietf.org/rfc/rfc4627.txt) requires 
        /// that the document stream begin with an array or object declaration
        /// and cannot begin with another type (e.g. integer). The user-supplied
        /// domNode (usually a JsonDomDocument) for the built DOM will thus have
        /// an array node or object node as its first (and usually only) child
        /// node. The user-supplied domeNode thus will not be the array or object node
        /// itself. If you want this array or object node to be the top of the
        /// DOM tree then simply detach the parent node and its child will become
        /// the top of the tree. 
        //////////////////////////////////////////////////////////////////////////
        class EAJSON_API JsonDomReader : public JsonReader
        {
        public:
            /// Default constructor
            /// The pAllocator is the allocator used for the JsonReader, which may
            /// be different from the allocator the user chooses to associated with
            /// the DOM that is build with the Build function.
            JsonDomReader(EA::Allocator::ICoreAllocator* pAllocator = NULL, size_t bufferBlockSize = 0);

            /// Build the DOM. 
            /// It is expected that you have called JsonReader base functions to 
            /// set up the class instance before doing this Parse.
            /// The domNode parameter may be any type of JsonDomNode, but typically the
            /// user would provide a JsonDomDocument node.
            /// The DOM is built into the domNode with the ICoreAllocator that is 
            /// associated with the domNode. If domNode's allocator is NULL then
            /// the allocator of this JsonDomReader class will be used.
            /// Upon error the DOM is left in its built state upon encountering the error.
            /// Builds the Dom with domNode's allocator. If domNode has no allocator
            /// then builds the Dom with this class' allocator.
            /// The JSON input data must be set via SetStream or SetString.
            Result Build(JsonDomNode& domNode);

        protected:
            void AddChildNode(JsonDomNode* pParentNode, JsonDomNode* pChildNode);
        };



        //////////////////////////////////////////////////////////////////////////
        /// IIJsonDomReaderCallback
        ///
        /// This is an interface for doing a SAX-like callback iteration of 
        /// the DOM. It is like the IContentHandler we have in IJsonCallbackReader
        /// except that it is for a DOM instead of a dynamically read document
        /// and thus the callbacks give you pointers directly to DomNodes.
        ///
        /// If you return true from an IIJsonDomReaderCallback function, recursive 
        /// parsing will continue. If you return false, no children of the 
        /// current node or its siblings will be iterated.
        ///
        /// This interface is not necessary for using the JSON DOM functionality;
        /// it's needed only for doing SAX-like iteration of a JSON DOM document.
        //////////////////////////////////////////////////////////////////////////
        class EAJSON_API IJsonDomReaderCallback
        {
        public:
            virtual ~IJsonDomReaderCallback() {}

            virtual bool BeginDocument(JsonDomDocument& domDocument);                                                           // This is called before anything else.
            virtual bool EndDocument(JsonDomDocument& domDocument);                                                             // This is called after all else.

            virtual bool BeginObject(JsonDomObject& domObject);                                                                 // This will be followed by BeginObjectValue or EndObject.
            virtual bool BeginObjectValue(JsonDomObject& domObject, const char* pName, size_t nNameLength, JsonDomObjectValue&);// If the Json content is valid then this will be followed by BeginObject, EndObject, BeginArray, Integer, Double, Bool, String, or Null.
            virtual bool EndObjectValue(JsonDomObject& domObject, JsonDomObjectValue&);                                         // 
            virtual bool EndObject(JsonDomObject& domObject);                                                                   // 

            virtual bool BeginArray(JsonDomArray& domArray);                                                                    // This will be followed by EndArray, BeginObject, BeginArray, Integer, Double, Bool, String, or Null.
            virtual bool EndArray(JsonDomArray& domArray);

            virtual bool Integer(JsonDomInteger&, int64_t value, const char* pText, size_t nLength);                            // 
            virtual bool Double(JsonDomDouble&, double value, const char* pText, size_t nLength);                               // 
            virtual bool Bool(JsonDomBool&, bool value, const char* pText, size_t nLength);                                     // 
            virtual bool String(JsonDomString&, const char* pValue, size_t nValueLength, const char* pText, size_t nLength);    // pValue will differ from pText if pText needed to have escape sequences decoded. pValue will remain valid for use after this function completes and until the user explicitly clears the Json buffer (see JsonReader).
            virtual bool Null(JsonDomNull&);                                                                                    // 
        };

    } // namespace Json

} // namespace EA





//////////////////////////////////////////////////////////////////////////
// Inlines
//////////////////////////////////////////////////////////////////////////

namespace EA
{
    namespace Json
    {
        // JsonDomNode
        
        inline JsonDomNode::JsonDomNode(NoInit)
            { }

        inline JsonDomNode::~JsonDomNode()
            { Clear(); }

        inline EventType JsonDomNode::GetNodeType() const
            { return mNodeType; }

        inline JsonDomNode* JsonDomNode::GetParent()
            { return mpParent; }

        inline void JsonDomNode::Clear()
        {
          //mNodeType = kETNone;    // I don't think this is right to do, as mNodeType is not really a variable value; it always must be the same value for a given instance.
            mName.clear();
          //mpParent = NULL;        // I don't think it's useful to do this. mpParent is a pointer which we don't own, so we don't need to free it.
        }

        inline const JsonDomNode* JsonDomNode::GetNode(const char8_t* pPath) const
            { return const_cast<JsonDomNode*>(this)->GetNode(pPath); }




        inline JsonDomObjectValue::~JsonDomObjectValue()
        {
            Clear();
        }

        inline JsonDomObject::~JsonDomObject()
        {
            Clear();
        }

        inline JsonDomArray::~JsonDomArray()
        {
            Clear();
        }





        // IJsonDomReaderCallback

        inline bool IJsonDomReaderCallback::BeginDocument(JsonDomDocument&)
            { return true; }

        inline bool IJsonDomReaderCallback::EndDocument(JsonDomDocument&)
            { return true; }

        inline bool IJsonDomReaderCallback::BeginObject(JsonDomObject&)
            { return true; }   

        inline bool IJsonDomReaderCallback::BeginObjectValue(JsonDomObject&, const char*, size_t, JsonDomObjectValue&)
            { return true; }

        inline bool IJsonDomReaderCallback::EndObjectValue(JsonDomObject&, JsonDomObjectValue&)
            { return true; }

        inline bool IJsonDomReaderCallback::EndObject(JsonDomObject&)
            { return true; }

        inline bool IJsonDomReaderCallback::BeginArray(JsonDomArray&)
            { return true; } 

        inline bool IJsonDomReaderCallback::EndArray(JsonDomArray&)
            { return true; }

        inline bool IJsonDomReaderCallback::Integer(JsonDomInteger&, int64_t, const char*, size_t)
            { return true; }

        inline bool IJsonDomReaderCallback::Double(JsonDomDouble&, double, const char*, size_t)
            { return true; } 

        inline bool IJsonDomReaderCallback::Bool(JsonDomBool&, bool, const char*, size_t)
            { return true; }     

        inline bool IJsonDomReaderCallback::String(JsonDomString&, const char*, size_t, const char*, size_t)
            { return true; }

        inline bool IJsonDomReaderCallback::Null(JsonDomNull&)
            { return true; }

    } // namespace Json

} // namespace EA


#ifdef _MSC_VER
    #pragma warning(pop)
#endif


#endif // Header include guard
