//
// Based on the original text_format.h file in the google.protobuf library
//
// Utilities for printing and parsing protocol messages in a simple xml format.
//
#ifndef GOOGLE_PROTOBUF_XML_FORMAT_H__
#define GOOGLE_PROTOBUF_XML_FORMAT_H__

#include <string>
#include <unordered_map>
#include "rapidxml-1.13/rapidxml.hpp"

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/stubs/hash.h>

namespace google {
namespace protobuf {

// This class implements protocol buffer xml format.
//
// This class is really a namespace that contains only static methods.
class LIBPROTOBUF_EXPORT XmlFormat {
 public:

  // Like MessageToDOM(), but outputs xml directly to a string.

  //Blaze fix: allow printing extra features, like XSLT style sheets, with an additional custom XmlPrintOptions struct:
  //static void PrintToXmlString(const Message& message, string* output);
  struct XmlPrintOptions
  {
      XmlPrintOptions() : minNumberPadLen(4) {}
      std::string styleSheetAttribs;
      size_t minNumberPadLen;
  };
  static void PrintToXmlString(const Message& message, string* output, const XmlPrintOptions& printOptions);

  // Like ...
  //Blaze fix: allow printing extra features, like XSLT style sheets, with an additional custom XmlPrintOptions struct:
  //static void MessageToDOM(const Message& message, rapidxml::xml_document<>* doc);
  static void MessageToDOM(const Message& message, rapidxml::xml_document<>* doc, const XmlPrintOptions& printOptions);

  // Class for those users which require more fine-grained control over how
  // a protobuffer message is printed out.
  class LIBPROTOBUF_EXPORT Printer {
   public:
    Printer();
    ~Printer();

    // Like XmlFormat::PrintToXmlString
    //Blaze fix: allow printing extra features, like XSLT style sheets, with an additional custom XmlPrintOptions struct:
    //void PrintToXmlString(const Message& message, string* output);
    void PrintToXmlString(const Message& message, string* output, const XmlPrintOptions& printOptions);

    // Like XmlFormat::MessageToDOM
    //Blaze fix: allow printing extra features, like XSLT style sheets, with an additional custom XmlPrintOptions struct:
    //void MessageToDOM(const Message& message, rapidxml::xml_document<>* doc);
    void MessageToDOM(const Message& message, rapidxml::xml_document<>* doc, const XmlPrintOptions& printOptions);


   private:
    // Internal Print method
    void PrintXml(const Message& message,
            rapidxml::xml_document<>* doc,
            rapidxml::xml_node<>* node,
            const XmlPrintOptions& printOptions);

    // Print a single field.
    void PrintXmlField(const Message& message,
                    const Reflection* reflection,
                    const FieldDescriptor* field,
                    rapidxml::xml_document<>* doc,
                    rapidxml::xml_node<>* node,
                    const XmlPrintOptions& printOptions);

    // Utility function to return the right name function based
    // on field type
    const string& GetXmlFieldName(const Message& message,//BLAZE CUSTOM UPDATE, return reference to the string, as a fix to original which tried to return a copy.
                        const Reflection* reflection,
                        const FieldDescriptor* field);

    // Outputs a textual representation of the value of the field supplied on
    // the message supplied or the default value if not set.
    void PrintXmlFieldValue(const Message& message,
                        const Reflection* reflection,
                        const FieldDescriptor* field,
                        int field_index,
                         rapidxml::xml_document<>* doc,
                         rapidxml::xml_node<>* node,
                        const XmlPrintOptions& printOptions);
  private:
      // Stores cached string representations for MessageToDom. TODO: this can be pre cached up front for efficiency:
      google::protobuf::hash_map<uint32, std::string> mUINT32StringMap;
      google::protobuf::hash_map<uint64, std::string> mUINT64StringMap;
      google::protobuf::hash_map<int32, std::string> mINT32StringMap;
      google::protobuf::hash_map<int64, std::string> mINT64StringMap;
      google::protobuf::hash_map<float, std::string> mFLOATStringMap;
      google::protobuf::hash_map<double, std::string> mDOUBLEStringMap;
  };


 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(XmlFormat);
};

}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_XML_FORMAT_H__

