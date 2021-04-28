// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <google/protobuf/compiler/blaze/cpp_map_field.h>
#include <google/protobuf/compiler/blaze/cpp_helpers.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/wire_format.h>
#include <google/protobuf/stubs/strutil.h>

#include <eadp/blaze/protobuf/blazeextensions.pb.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

bool IsProto3Field(const FieldDescriptor* field_descriptor) {
  const FileDescriptor* file_descriptor = field_descriptor->file();
  return file_descriptor->syntax() == FileDescriptor::SYNTAX_PROTO3;
}

void SetMessageVariables(const FieldDescriptor* descriptor,
                        std::map<string, string>* variables,
                         const Options& options, bool valueIsComplex, bool isEnumKey, bool hasIgnoreCase) {
  SetCommonFieldVariables(descriptor, variables, options);
  (*variables)["type"] = FieldMessageTypeName(descriptor);
  (*variables)["stream_writer"] =
      (*variables)["declared_type"] +
      (HasFastArraySerialization(descriptor->message_type()->file(), options)
           ? "MaybeToArray"
           : "");
  (*variables)["full_name"] = descriptor->full_name();

  const FieldDescriptor* key =
      descriptor->message_type()->FindFieldByName("key");
  const FieldDescriptor* val =
      descriptor->message_type()->FindFieldByName("value");
  
  std::string keyEnumType;
  std::string key_cpp_type = PrimitiveTypeName(key->cpp_type());
  //override 
  {
      const eadp::blaze::protobuf::FieldOptions& fieldOptions = descriptor->options().GetExtension(eadp::blaze::protobuf::field_options);
      if (!fieldOptions.key_tdf_type().empty())
      {
          if (isEnumKey)
          {
              keyEnumType = DotsToColons(fieldOptions.key_tdf_type());
              keyEnumType = StringReplace(keyEnumType, "eadp::blaze", "Blaze", false);
              key_cpp_type = keyEnumType;
          }
          else
          {
              key_cpp_type = fieldOptions.key_tdf_type();
          }
      }
  }
  (*variables)["key_cpp"] = key_cpp_type.c_str();

  std::string val_cpp_type;
  switch (val->cpp_type()) {
    case FieldDescriptor::CPPTYPE_MESSAGE:
    {
        std::string tdfType = getTdfType(val);
        val_cpp_type = !tdfType.empty() ? tdfType.c_str() : FieldMessageTypeName(val);
        (*variables)["val_cpp"] = val_cpp_type.c_str();
        (*variables)["wrapper"] = "EntryWrapper";
        break;
    }
    case FieldDescriptor::CPPTYPE_ENUM:
      val_cpp_type = ClassName(val->enum_type(), true);
      (*variables)["val_cpp"] = val_cpp_type.c_str();
      (*variables)["wrapper"] = "EnumEntryWrapper";
      break;
    default:
      val_cpp_type = PrimitiveTypeName(val->cpp_type());
      // override
      {
          const eadp::blaze::protobuf::FieldOptions& fieldOptions = descriptor->options().GetExtension(eadp::blaze::protobuf::field_options);
          if (!fieldOptions.value_tdf_type().empty())
          {
              val_cpp_type = fieldOptions.value_tdf_type();
          }
      }
      (*variables)["val_cpp"] = val_cpp_type.c_str();
      (*variables)["wrapper"] = "EntryWrapper";
  }
  (*variables)["key_wire_type"] =
      "::google::protobuf::internal::WireFormatLite::TYPE_" +
      ToUpper(DeclaredTypeMethodName(key->type()));
  (*variables)["val_wire_type"] =
      "::google::protobuf::internal::WireFormatLite::TYPE_" +
      ToUpper(DeclaredTypeMethodName(val->type()));
  (*variables)["map_classname"] = ClassName(descriptor->message_type(), false);
  (*variables)["number"] = SimpleItoa(descriptor->number());
  (*variables)["tag"] = SimpleItoa(internal::WireFormat::MakeTag(descriptor));

  if (HasDescriptorMethods(descriptor->file(), options)) {
    (*variables)["lite"] = "";
  } else {
    (*variables)["lite"] = "Lite";
  }

  if (!IsProto3Field(descriptor) &&
      val->type() == FieldDescriptor::TYPE_ENUM) {
    const EnumValueDescriptor* default_value = val->default_value_enum();
    (*variables)["default_enum_value"] = Int32ToString(default_value->number());
  } else {
    (*variables)["default_enum_value"] = "0";
  }

  (*variables)["entry_key_getter"] = "entry.getKey()";
  (*variables)["entry_key_setter"] = "entry.setKey(iter->first);";

  (*variables)["entry_value_getter"] = "entry.getValue()";
  (*variables)["entry_value_setter"] = valueIsComplex ? "entry.setValue(*(iter->second));" : "entry.setValue(iter->second);";

  const FieldDescriptor* value_field = descriptor->message_type()->FindFieldByName("value");
  if (IsProxyMessage(value_field->message_type()))
  {
      (*variables)["entry_value_getter"] = "entry.getValue().getValue()";
      (*variables)["entry_value_setter"] = "entry.getValue().setValue(*(iter->second));";
  }

  (*variables)["blaze_key_enum_conditional"] = "{";
  if (!keyEnumType.empty())
  {
      (*variables)["blaze_key_enum_conditional"] = StrCat("if (", keyEnumType.c_str(), "_IsValid(entry.getKey())) {");
      (*variables)["entry_key_getter"] = StrCat("static_cast<", keyEnumType.c_str(), ">(entry.getKey())"); 
  }
  
  (*variables)["map_type"] = StrCat(valueIsComplex ? "EA::TDF::TdfStructMap" : "EA::TDF::TdfPrimitiveMap",
      "< ", key_cpp_type.c_str(),
      " , ", val_cpp_type.c_str(),
      (hasIgnoreCase ? " , EA::TDF::TdfStringCompareIgnoreCase, true" : ""),
      " >");
}

bool doesUseComplexMap(const FieldDescriptor* descriptor)
{
    if (descriptor->type() == FieldDescriptor::TYPE_BYTES)
        return true;

    if (descriptor->type() == FieldDescriptor::TYPE_MESSAGE)
    {
        // Messages use complex map but have some exceptions. 
        if (IsTdfTypeBitfield(descriptor->message_type()) || IsObjectTypeMessage(descriptor->message_type()) || IsObjectIdMessage(descriptor->message_type()) || IsTimeValueMessage(descriptor->message_type()))
            return false;
        
        return true;
    }

    return false;
}

MapFieldGenerator::MapFieldGenerator(const FieldDescriptor* descriptor,
                                     const Options& options)
    : FieldGenerator(options),
      descriptor_(descriptor),
      dependent_field_(options.proto_h && IsFieldDependent(descriptor)),
      value_is_complex_(doesUseComplexMap(descriptor->message_type()->FindFieldByName("value"))){
    
    bool isEnumKey = false;
    bool hasIgnoreCase = false;
    const FieldDescriptor* key_field = descriptor_->message_type()->FindFieldByName("key");
    const eadp::blaze::protobuf::FieldOptions& fieldOptions = descriptor_->options().GetExtension(eadp::blaze::protobuf::field_options);
    if (key_field->type() == FieldDescriptor::TYPE_INT32)
    {
        isEnumKey = fieldOptions.key_is_enum();
    }
    else if (key_field->type() == FieldDescriptor::TYPE_STRING)
    {
        hasIgnoreCase = fieldOptions.has_ignore_case();
    }

  SetMessageVariables(descriptor, &variables_, options, value_is_complex_, isEnumKey, hasIgnoreCase);
}

MapFieldGenerator::~MapFieldGenerator() {}

void MapFieldGenerator::
GeneratePrivateMembers(io::Printer* printer) const {
  
    printer->Print(variables_,
        "$map_type$ m$name$;\n");
}

void MapFieldGenerator::
GenerateAccessorDeclarations(io::Printer* printer) const {
  
  printer->Print(variables_,
      "$deprecated_attr$ $map_type$& get$name$();\n"
      "$deprecated_attr$const $map_type$& get$name$() const;\n"
      "$deprecated_attr$void set$name$(const $map_type$&);\n");
}

void MapFieldGenerator::
GenerateInlineAccessorDefinitions(io::Printer* printer,
                                  bool is_inline) const {
  std::map<string, string> variables(variables_);
  variables["inline"] = is_inline ? "inline " : "";
  
  printer->Print(variables,
      "$inline$ $map_type$& $classname$::get$name$() { return m$name$; }\n"
      "$inline$const $map_type$& $classname$::get$name$() const { return m$name$; }\n"
      "$inline$void $classname$::set$name$(const $map_type$& value) { value.copyInto(m$name$);}\n"//
      "  // @@protoc_insertion_point(field_get:$full_name$)\n");
}

void MapFieldGenerator::
GenerateClearingCode(io::Printer* printer) const {
  std::map<string, string> variables(variables_);
  variables["this_message"] = dependent_field_ ? DependentBaseDownCast() : "";
  printer->Print(variables, "get$name$().clearMap();\n");
}

void MapFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  printer->Print(variables_, "$name$_.MergeFrom(from.$name$_);\n");
}

void MapFieldGenerator::
GenerateSwappingCode(io::Printer* printer) const {
  printer->Print(variables_, "$name$_.Swap(&other->$name$_);\n");
}

void MapFieldGenerator::
GenerateConstructorCode(io::Printer* printer) const {
  if (HasDescriptorMethods(descriptor_->file(), options_)) {
    printer->Print(variables_,
        "$name$_.SetAssignDescriptorCallback(\n"
        "    protobuf_AssignDescriptorsOnce);\n"
        "$name$_.SetEntryDescriptor(\n"
        "    &$type$_descriptor_);\n");
  }
}

void MapFieldGenerator::
GenerateMergeFromCodedStream(io::Printer* printer) const {
    const FieldDescriptor* key_field =
        descriptor_->message_type()->FindFieldByName("key");
  const FieldDescriptor* value_field =
      descriptor_->message_type()->FindFieldByName("value");
  bool using_entry = false;
  
  if (IsProto3Field(descriptor_) ||
      value_field->type() != FieldDescriptor::TYPE_ENUM) {
      
      printer->Print(variables_,
          "$map_classname$ entry;\n"
          "DO_(::google::protobuf::internal::WireFormatLite::ReadMessage(\n"
          "    input, &entry));\n");

      printer->Print(variables_, "$blaze_key_enum_conditional$\n");
      printer->Indent();

      if (value_is_complex_)
      {
          printer->Print(variables_,
              "auto mapValue = m$name$.allocate_element();\n"
              "$entry_value_getter$.copyInto(*mapValue);\n"
              "m$name$[$entry_key_getter$] = mapValue;\n");
      }
      else
      {
          printer->Print(variables_,
              "m$name$[$entry_key_getter$] = $entry_value_getter$;\n");
      }

      printer->Outdent();
      printer->Print("}\n");
  } 

  if (key_field->type() == FieldDescriptor::TYPE_STRING) {
    GenerateUtf8CheckCodeForString(
    key_field, options_, true, variables_,
        "$entry_key_getter$, entry.getKeySize(), ", printer);
  }
  if (value_field->type() == FieldDescriptor::TYPE_STRING) {
    GenerateUtf8CheckCodeForString(value_field, options_, true, variables_,
        "$entry_value_getter$, entry.getValueSize(), ", printer);
  }

  // If entry is allocated by arena, its desctructor should be avoided.
  if (using_entry && SupportsArenas(descriptor_)) {
    printer->Print(variables_,
        "if (entry->GetArena() != NULL) entry.release();\n");
  }
}

static void GenerateSerializationLoop(io::Printer* printer,
                                      const std::map<string, string>& variables,
                                      bool supports_arenas,
                                      const string& utf8_check,
                                      const string& loop_header,
                                      const string& ptr,
                                      bool loop_via_iterators,
                                      bool valueIsComplex) {
  
  printer->Print(variables,loop_header.c_str());
  printer->Print("{\n");
  printer->Indent();

  printer->Print(variables, "$map_classname$ entry;\n"
      "$entry_key_setter$\n"
      "$entry_value_setter$\n");
  
  printer->Print(variables, "entry.ByteSizeLong(); //Need to force a byte size calculation due to temporary entry message\n" 
      "$write_entry$;\n");

  // If entry is allocated by arena, its desctructor should be avoided.
  if (supports_arenas) {
    printer->Print(
        "if (entry->GetArena() != NULL) {\n"
        "  entry.release();\n"
        "}\n");
  }

  if (!utf8_check.empty()) {
    // If loop_via_iterators is true then ptr is actually an iterator, and we
    // create a pointer by prefixing it with "&*".
    printer->Print(
        StrCat(utf8_check, "(", (loop_via_iterators ? "&*" : ""), ptr, ");\n")
            .c_str());
  }

  printer->Outdent();
  printer->Print(
      "}\n");
}

void MapFieldGenerator::
GenerateSerializeWithCachedSizes(io::Printer* printer) const {
  std::map<string, string> variables(variables_);
  variables["write_entry"] = "::google::protobuf::internal::WireFormatLite::Write" +
                             variables["stream_writer"] + "(\n            " +
                             variables["number"] + ", entry, output)";
  variables["deterministic"] = "output->IsSerializationDeterminstic()";
  GenerateSerializeWithCachedSizes(printer, variables);
}

void MapFieldGenerator::
GenerateSerializeWithCachedSizesToArray(io::Printer* printer) const {
  std::map<string, string> variables(variables_);
  variables["write_entry"] =
      "target = ::google::protobuf::internal::WireFormatLite::\n"
      "                   InternalWrite" + variables["declared_type"] +
      "ToArray(\n                       " + variables["number"] +
      ", entry, false, target)";
  variables["deterministic"] = "deterministic";
  GenerateSerializeWithCachedSizes(printer, variables);
}

void MapFieldGenerator::GenerateSerializeWithCachedSizes(
    io::Printer* printer, const std::map<string, string>& variables) const {
  printer->Print(variables,
      "if (!get$name$().empty()) {\n");
  printer->Indent();
  
#if DEFAULT_PROTOC
  printer->Print(variables,
      "typedef ::google::protobuf::Map< $key_cpp$, $val_cpp$ >::const_pointer\n"
      "    ConstPtr;\n");
  if (string_key) {
    printer->Print(variables,
        "typedef ConstPtr SortItem;\n"
        "typedef ::google::protobuf::internal::"
        "CompareByDerefFirst<SortItem> Less;\n");
  } else {
    printer->Print(variables,
        "typedef ::google::protobuf::internal::SortItem< $key_cpp$, ConstPtr > "
        "SortItem;\n"
        "typedef ::google::protobuf::internal::CompareByFirstField<SortItem> Less;\n");
  }
#endif
  string utf8_check;
#if DEFAULT_PROTOC
  if (string_key || string_value) {
    printer->Print(
        "struct Utf8Check {\n"
        "  static void Check(const char8_t* data, uint32_t length) {\n");
    printer->Indent();
    printer->Indent();
#if DEFAULT_PROTOC
    if (string_key) {
      GenerateUtf8CheckCodeForString(key_field, options_, false, variables,
                                     "data, length,\n",
                                     printer);
    }
    if (string_value) {
      GenerateUtf8CheckCodeForString(value_field, options_, false, variables,
                                     "p->second.data(), p->second.length(),\n",
                                     printer);
    }
#endif
    printer->Outdent();
    printer->Outdent();
    printer->Print(
        "  }\n"
        "};\n");
    utf8_check = "";// "Utf8Check::Check";
  }
  printer->Print(variables,
      "\n"
      "if ($deterministic$ &&\n"
      "    this->$name$().size() > 1) {\n"
      "  ::google::protobuf::scoped_array<SortItem> items(\n"
      "      new SortItem[this->$name$().size()]);\n"
      "  typedef ::google::protobuf::Map< $key_cpp$, $val_cpp$ >::size_type size_type;\n"
      "  size_type n = 0;\n"
      "  for (::google::protobuf::Map< $key_cpp$, $val_cpp$ >::const_iterator\n"
      "      it = this->$name$().begin();\n"
      "      it != this->$name$().end(); ++it, ++n) {\n"
      "    items[n] = SortItem(&*it);\n"
      "  }\n"
      "  ::std::sort(&items[0], &items[n], Less());\n");
  printer->Indent();
  GenerateSerializationLoop(printer, variables, SupportsArenas(descriptor_),
                            utf8_check, "for (size_type i = 0; i < n; i++)",
                            string_key ? "items[i]" : "items[i].second", false, value_is_complex_);
  printer->Outdent();
  printer->Print(
      "} else {\n");
  printer->Indent();
#endif
  GenerateSerializationLoop(
      printer, variables, SupportsArenas(descriptor_), utf8_check,
      "for (auto iter = get$name$().begin(); iter != get$name$().end(); ++iter)\n",
      "it", true, value_is_complex_);
  //printer->Outdent();
  //printer->Print("}\n");
  printer->Outdent();
  printer->Print("}\n");
}

void MapFieldGenerator::
GenerateByteSize(io::Printer* printer) const {
  printer->Print(variables_,
      "total_size += $tag_size$ * get$name$().size();\n"
      "{\n"
      "  for (auto iter = get$name$().begin(); iter != get$name$().end(); ++iter) {\n");

  // If entry is allocated by arena, its desctructor should be avoided.
  if (SupportsArenas(descriptor_)) {
    printer->Print(variables_,
        "    if (entry.get() != NULL && entry->GetArena() != NULL) {\n"
        "      entry.release();\n"
        "    }\n");
  }
  
  printer->Print(variables_,
      "     $map_classname$ entry;\n"
      "     $entry_key_setter$\n"
      "     $entry_value_setter$\n");
  
  printer->Print(variables_,
      "     total_size += ::google::protobuf::internal::WireFormatLite::\n"
      "        $declared_type$Size(entry);\n"
      "  }\n");

  // If entry is allocated by arena, its desctructor should be avoided.
  if (SupportsArenas(descriptor_)) {
    printer->Print(variables_,
        "  if (entry.get() != NULL && entry->GetArena() != NULL) {\n"
        "    entry.release();\n"
        "  }\n");
  }

  printer->Print("}\n");
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
