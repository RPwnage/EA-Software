#include <sstream>
#include <iostream>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>

#include "rapidxml-1.13/rapidxml.hpp"
#include "rapidxml-1.13/rapidxml_print.hpp"

#include "xml_format.h"
#include <inttypes.h>


#define XML_TRUE_STRING "true"
#define XML_FALSE_STRING "false"

using namespace std;

namespace google {
namespace protobuf {



XmlFormat::Printer::Printer() {}
XmlFormat::Printer::~Printer() {}

void XmlFormat::Printer::PrintToXmlString(const Message& message,
                                        string* output, const XmlPrintOptions& printOptions) {
  GOOGLE_DCHECK(output) << "output specified is nullptr";

  output->clear();

  // create the xml dom
  rapidxml::xml_document<> doc;

  MessageToDOM(message,&doc, printOptions);

  std::stringstream ss;
  ss << doc;
  *output = ss.str();

}


void XmlFormat::Printer::MessageToDOM(const Message& message, rapidxml::xml_document<>* doc, const XmlPrintOptions& printOptions) {
    // xml version and encoding
    rapidxml::xml_node<>* xml_decl = doc->allocate_node(rapidxml::node_declaration);
    xml_decl->append_attribute(doc->allocate_attribute("version", "1.0"));
    xml_decl->append_attribute(doc->allocate_attribute("encoding", "utf-8"));
    doc->append_node(xml_decl);

    //Blaze fix: allow printing extra features, like XSLT style sheets, with an additional custom XmlPrintOptions struct:
    if (!printOptions.styleSheetAttribs.empty())
    {
        rapidxml::xml_node<>* xml_style = doc->allocate_node(rapidxml::node_pi);
        xml_style->name("xml-stylesheet");
        xml_style->value(printOptions.styleSheetAttribs.c_str());
        doc->append_node(xml_style);
    }

    // the root node of the protobuf xml is the name of the protobuf container
    rapidxml::xml_node<> *root_node = doc->allocate_node(rapidxml::node_element, message.GetDescriptor()->name().c_str());
    doc->append_node(root_node);

    PrintXml(message, doc, root_node, printOptions);
}

void XmlFormat::Printer::PrintXml(const Message& message,
                                rapidxml::xml_document<>* doc,
                                rapidxml::xml_node<>* node,
                                const XmlPrintOptions& printOptions) {

    const Reflection* reflection = message.GetReflection();
    vector<const FieldDescriptor*> fields;

    //Fix for Blaze: orig bugged version commented out below, would skip fields having a value that happens to equal the default value. TODO: if we do want *option* of omitting default valued fields, this should be specified as a passed in param:
    //reflection->ListFields(message, &fields);//Fixed version below
    int fieldCount = message.GetDescriptor()->field_count();
    for (int i = 0; i < fieldCount; ++i)
    {
        fields.push_back(message.GetDescriptor()->field(i));
    }

    for (unsigned int i = 0; i < fields.size(); i++) {
        PrintXmlField(message,reflection, fields[i], doc, node, printOptions);
    }

}


void XmlFormat::Printer::PrintXmlField(const Message& message,
                                     const Reflection* reflection,
                                     const FieldDescriptor* field,
                                     rapidxml::xml_document<>* doc,
                                     rapidxml::xml_node<>* node,
                                    const XmlPrintOptions& printOptions) {
    int count = 0;
    if (field->is_repeated()) {
        count = reflection->FieldSize(message, field);
        //Fix for Blaze: orig bugged version commented out below, would skip fields having a value that happens to equal the default value. TODO: if we do want *option* of omitting default valued fields, this should be specified as a passed in param:
    //} else if (reflection->HasField(message, field)) {//Fixed version below.
    } else {
        count = 1;
    }

    for (int j = 0; j < count; ++j) {
        // Write the field value.
        int field_index = j;
        if (!field->is_repeated()) {
            field_index = -1;
        }

        PrintXmlFieldValue(message, reflection, field, field_index, doc, node, printOptions);
    }
}


const string& XmlFormat::Printer::GetXmlFieldName(const Message& message,
                                         const Reflection* reflection,
                                         const FieldDescriptor* field) {
    if (field->is_extension()) {
        // We special-case MessageSet elements for compatibility with proto1.
        if (field->containing_type()->options().message_set_wire_format()
                && field->type() == FieldDescriptor::TYPE_MESSAGE
                && field->is_optional()
                && field->extension_scope() == field->message_type()) {
            return field->message_type()->full_name();
        } else {
            return field->full_name();
        }
    } else {
        if (field->type() == FieldDescriptor::TYPE_GROUP) {
            // Groups must be serialized with their original capitalization.
            return field->message_type()->name();
        } else {
            return field->name();
        }
    }
}

void XmlFormat::Printer::PrintXmlFieldValue(
    const Message& message,
    const Reflection* reflection,
    const FieldDescriptor* field,
    int field_index,
    rapidxml::xml_document<>* doc,
    rapidxml::xml_node<>* node,
    const XmlPrintOptions& printOptions) {

    GOOGLE_DCHECK(field->is_repeated() || (field_index == -1))
            << "field_index must be -1 for non-repeated fields";

    switch (field->cpp_type()) {

        //tm use the preprocessor to generate the numerical value cases
        // replace the google used string methods with using a stringstream
#define OUTPUT_FIELD(CPPTYPE, METHOD, NUM_TYPE)                         \
      case FieldDescriptor::CPPTYPE_##CPPTYPE: {                              \
        NUM_TYPE value = field->is_repeated() ?                      \
          reflection->GetRepeated##METHOD(message, field, field_index) :     \
          reflection->Get##METHOD(message, field);                          \
        std::string& numAsStr = m##CPPTYPE##StringMap[value];\
        if (numAsStr.empty()) {\
            stringstream number_stream; \
            number_stream << value; \
            numAsStr = number_stream.str().c_str(); \
            while (numAsStr.size() < printOptions.minNumberPadLen) {\
                numAsStr = "0" + numAsStr;\
            }\
        }\
        rapidxml::xml_node<> *string_node = doc->allocate_node(              \
          rapidxml::node_element,                                      \
          GetXmlFieldName(message, reflection, field).c_str(),         \
          numAsStr.c_str());                                              \
        node->append_node(string_node);                                      \
        break;                                                               \
      }

      OUTPUT_FIELD( INT32,  Int32, int32_t);
      OUTPUT_FIELD( INT64,  Int64, int64_t);
      OUTPUT_FIELD(UINT32, UInt32, uint32_t);
      OUTPUT_FIELD(UINT64, UInt64, uint64_t);
      OUTPUT_FIELD( FLOAT,  Float, float);
      OUTPUT_FIELD(DOUBLE, Double, double);
#undef OUTPUT_FIELD

    case FieldDescriptor::CPPTYPE_STRING: {
        string scratch;
        const string& value = field->is_repeated() ?
            reflection->GetRepeatedStringReference(
              message, field, field_index, &scratch) :
            reflection->GetStringReference(message, field, &scratch);

        rapidxml::xml_node<> *string_node = doc->allocate_node(rapidxml::node_element,
                GetXmlFieldName(message, reflection, field).c_str(),
                value.c_str());
        node->append_node(string_node);

        break;
    }

    case FieldDescriptor::CPPTYPE_BOOL: {
        if (field->is_repeated()) {
            if (reflection->GetRepeatedBool(message, field, field_index)) {
                rapidxml::xml_node<> *bool_node = doc->allocate_node(rapidxml::node_element,
                    GetXmlFieldName(message, reflection, field).c_str(),
                    XML_TRUE_STRING);
                node->append_node(bool_node);
            } else {
                rapidxml::xml_node<> *bool_node = doc->allocate_node(rapidxml::node_element,
                    GetXmlFieldName(message, reflection, field).c_str(),
                    XML_FALSE_STRING);
                node->append_node(bool_node);
            }
        } else {
            if (reflection->GetBool(message,field)) {
                rapidxml::xml_node<> *bool_node = doc->allocate_node(rapidxml::node_element,
                    GetXmlFieldName(message, reflection, field).c_str(),
                    XML_TRUE_STRING);
                node->append_node(bool_node);
            } else {
                rapidxml::xml_node<> *bool_node = doc->allocate_node(rapidxml::node_element,
                    GetXmlFieldName(message, reflection, field).c_str(),
                    XML_FALSE_STRING);
                node->append_node(bool_node);
            }
        }
        break;
    }

    case FieldDescriptor::CPPTYPE_ENUM: {
        const string& value = field->is_repeated() ?
          reflection->GetRepeatedEnum(message, field, field_index)->name() :
          reflection->GetEnum(message, field)->name();
        rapidxml::xml_node<> *enum_node = doc->allocate_node(rapidxml::node_element,
                GetXmlFieldName(message, reflection, field).c_str(),
                value.c_str());
        node->append_node(enum_node);
        break;
    }
    case FieldDescriptor::CPPTYPE_MESSAGE: {
        // create the child node and recurse
        rapidxml::xml_node<> *message_node = doc->allocate_node(rapidxml::node_element, field->name().c_str());
        node->append_node(message_node);

        PrintXml(field->is_repeated() ?
                          reflection->GetRepeatedMessage(message, field, field_index) :
                          reflection->GetMessage(message, field),
                 doc, message_node, printOptions);
        break;
    }
    }
}



/* static */ void XmlFormat::PrintToXmlString(
    const Message& message, string* output, const XmlPrintOptions& printOptions) {
  Printer().PrintToXmlString(message, output, printOptions);
}

/* static */ void XmlFormat::MessageToDOM(
    const Message& message, rapidxml::xml_document<>* doc, const XmlPrintOptions& printOptions) {
    Printer().MessageToDOM(message, doc, printOptions);
}


}  // namespace protobuf
}  // namespace google
