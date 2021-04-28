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

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <google/protobuf/compiler/blaze/cpp_primitive_field.h>
#include <google/protobuf/compiler/blaze/cpp_helpers.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/wire_format.h>
#include <google/protobuf/stubs/strutil.h>

#include <eadp/blaze/protobuf/blazeextensions.pb.h>


namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

using internal::WireFormatLite;

namespace {

// For encodings with fixed sizes, returns that size in bytes.  Otherwise
// returns -1.
int FixedSize(FieldDescriptor::Type type) {
  switch (type) {
    case FieldDescriptor::TYPE_INT32   : return -1;
    case FieldDescriptor::TYPE_INT64   : return -1;
    case FieldDescriptor::TYPE_UINT32  : return -1;
    case FieldDescriptor::TYPE_UINT64  : return -1;
    case FieldDescriptor::TYPE_SINT32  : return -1;
    case FieldDescriptor::TYPE_SINT64  : return -1;
    case FieldDescriptor::TYPE_FIXED32 : return WireFormatLite::kFixed32Size;
    case FieldDescriptor::TYPE_FIXED64 : return WireFormatLite::kFixed64Size;
    case FieldDescriptor::TYPE_SFIXED32: return WireFormatLite::kSFixed32Size;
    case FieldDescriptor::TYPE_SFIXED64: return WireFormatLite::kSFixed64Size;
    case FieldDescriptor::TYPE_FLOAT   : return WireFormatLite::kFloatSize;
    case FieldDescriptor::TYPE_DOUBLE  : return WireFormatLite::kDoubleSize;

    case FieldDescriptor::TYPE_BOOL    : return WireFormatLite::kBoolSize;
    case FieldDescriptor::TYPE_ENUM    : return -1;

    case FieldDescriptor::TYPE_STRING  : return -1;
    case FieldDescriptor::TYPE_BYTES   : return -1;
    case FieldDescriptor::TYPE_GROUP   : return -1;
    case FieldDescriptor::TYPE_MESSAGE : return -1;

    // No default because we want the compiler to complain if any new
    // types are added.
  }
  GOOGLE_LOG(FATAL) << "Can't get here.";
  return -1;
}

void SetPrimitiveVariables(const FieldDescriptor* descriptor,
    std::map<string, string>* variables,
                           const Options& options) {
  SetCommonFieldVariables(descriptor, variables, options);
  
  (*variables)["value_tdf_type"] = "";
  const eadp::blaze::protobuf::FieldOptions& fieldOptions = descriptor->options().GetExtension(eadp::blaze::protobuf::field_options);
  if (!fieldOptions.value_tdf_type().empty())
  {
      (*variables)["value_tdf_type"] = fieldOptions.value_tdf_type();
  }
 
  (*variables)["type"] = PrimitiveTypeName(descriptor->cpp_type());
  (*variables)["default"] = DefaultValue(descriptor);
  (*variables)["tag"] = SimpleItoa(internal::WireFormat::MakeTag(descriptor));
  int fixed_size = FixedSize(descriptor->type());
  if (fixed_size != -1) {
    (*variables)["fixed_size"] = SimpleItoa(fixed_size);
  }
  (*variables)["wire_format_field_type"] =
      "::google::protobuf::internal::WireFormatLite::" + FieldDescriptorProto_Type_Name(
          static_cast<FieldDescriptorProto_Type>(descriptor->type()));
  (*variables)["full_name"] = descriptor->full_name();
}

}  // namespace

// ===================================================================

PrimitiveFieldGenerator::PrimitiveFieldGenerator(
    const FieldDescriptor* descriptor, const Options& options)
    : FieldGenerator(options), descriptor_(descriptor), use_intermediate_(true) {
  SetPrimitiveVariables(descriptor, &variables_, options);

  const eadp::blaze::protobuf::FieldOptions& fieldOptions = descriptor->options().GetExtension(eadp::blaze::protobuf::field_options);
  use_intermediate_ = !fieldOptions.prim_type_is_same();
}

PrimitiveFieldGenerator::~PrimitiveFieldGenerator() {}

void PrimitiveFieldGenerator::
GeneratePrivateMembers(io::Printer* printer) const {
  printer->Print(variables_, "$type$ m$name$ = 0;\n"); 
}

void PrimitiveFieldGenerator::
GenerateAccessorDeclarations(io::Printer* printer) const {
  printer->Print(variables_,
    "$deprecated_attr$$type$ get$name$() const;\n"
    "$deprecated_attr$void set$name$($type$ value);\n");
}

void PrimitiveFieldGenerator::
GenerateInlineAccessorDefinitions(io::Printer* printer, bool is_inline) const {
    std::map<string, string> variables(variables_);
  variables["inline"] = is_inline ? "inline " : "";
  printer->Print(variables,
    "$inline$ $type$ $classname$::get$name$() const {\n"
    "  // @@protoc_insertion_point(field_get:$full_name$)\n"
    "  return m$name$;\n"
    "}\n"
    "$inline$ void $classname$::set$name$($type$ value) {\n"
    "  $set_hasbit$\n"
    "  m$name$ = value;\n"
    "  // @@protoc_insertion_point(field_set:$full_name$)\n"
    "}\n");
}

void PrimitiveFieldGenerator::
GenerateClearingCode(io::Printer* printer) const {
  printer->Print(variables_, "m$name$ = $default$;\n");
}

void PrimitiveFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  printer->Print(variables_, "set_$name$(from.$name$());\n");
}

void PrimitiveFieldGenerator::
GenerateSwappingCode(io::Printer* printer) const {
  printer->Print(variables_, "std::swap($name$, other->$name$);\n");
}

void PrimitiveFieldGenerator::
GenerateConstructorCode(io::Printer* printer) const {
  printer->Print(variables_, "$name$ = $default$;\n");
}

void PrimitiveFieldGenerator::
GenerateMergeFromCodedStream(io::Printer* printer) const {

if (use_intermediate_)
{
    printer->Print(variables_,
        "$type$ value = $default$;\n"
        "DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<\n"
        "         $type$, $wire_format_field_type$>(\n"
        "       input, &value)));\n"
        " set$name$(value);"
        "$set_hasbit$\n");
}
else
{
    printer->Print(variables_,
        "DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<\n"
        "         $type$, $wire_format_field_type$>(\n"
        "       input, &m$name$)));"
        "$set_hasbit$\n");
}
}

void PrimitiveFieldGenerator::
GenerateSerializeWithCachedSizes(io::Printer* printer) const {
  printer->Print(variables_,
    "::google::protobuf::internal::WireFormatLite::Write$declared_type$("
      "$number$, this->get$name$(), output);\n");
}

void PrimitiveFieldGenerator::
GenerateSerializeWithCachedSizesToArray(io::Printer* printer) const {
  printer->Print(variables_,
    "target = ::google::protobuf::internal::WireFormatLite::Write$declared_type$ToArray("
      "$number$, this->get$name$(), target);\n");
}

void PrimitiveFieldGenerator::
GenerateByteSize(io::Printer* printer) const {
  int fixed_size = FixedSize(descriptor_->type());
  if (fixed_size == -1) {
    printer->Print(variables_,
      "total_size += $tag_size$ +\n"
      "  ::google::protobuf::internal::WireFormatLite::$declared_type$Size(\n"
      "    this->get$name$());\n");
  } else {
    printer->Print(variables_,
      "total_size += $tag_size$ + $fixed_size$;\n");
  }
}

// ===================================================================

PrimitiveOneofFieldGenerator::
PrimitiveOneofFieldGenerator(const FieldDescriptor* descriptor,
                             const Options& options)
  : PrimitiveFieldGenerator(descriptor, options) {
  SetCommonOneofFieldVariables(descriptor, &variables_);
}

PrimitiveOneofFieldGenerator::~PrimitiveOneofFieldGenerator() {}

void PrimitiveOneofFieldGenerator::
GenerateInlineAccessorDefinitions(io::Printer* printer, bool is_inline) const {
    std::map<string, string> variables(variables_);
  variables["inline"] = is_inline ? "inline " : "";
  printer->Print(variables,
    "$inline$ $type$ $classname$::$name$() const {\n"
    "  // @@protoc_insertion_point(field_get:$full_name$)\n"
    "  if (has_$name$()) {\n"
    "    return $oneof_prefix$$name$;\n"
    "  }\n"
    "  return $default$;\n"
    "}\n"
    "$inline$ void $classname$::set_$name$($type$ value) {\n"
    "  if (!has_$name$()) {\n"
    "    clear_$oneof_name$();\n"
    "    set_has_$name$();\n"
    "  }\n"
    "  $oneof_prefix$$name$ = value;\n"
    "  // @@protoc_insertion_point(field_set:$full_name$)\n"
    "}\n");
}

void PrimitiveOneofFieldGenerator::
GenerateClearingCode(io::Printer* printer) const {
  printer->Print(variables_, "$oneof_prefix$$name$ = $default$;\n");
}

void PrimitiveOneofFieldGenerator::
GenerateSwappingCode(io::Printer* printer) const {
  // Don't print any swapping code. Swapping the union will swap this field.
}

void PrimitiveOneofFieldGenerator::
GenerateConstructorCode(io::Printer* printer) const {
  printer->Print(
      variables_,
      "  $classname$_default_oneof_instance_->$name$ = $default$;\n");
}

void PrimitiveOneofFieldGenerator::
GenerateMergeFromCodedStream(io::Printer* printer) const {
    printer->Print(variables_,
    "clear_$oneof_name$();\n"
    "$type$ value = $default$;\n"
    "DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<\n"
    "         $type$, $wire_format_field_type$>(\n"
    "       input, &value)));\n"
    " set$name$(value);\n");
}

// ===================================================================

RepeatedPrimitiveFieldGenerator::RepeatedPrimitiveFieldGenerator(
    const FieldDescriptor* descriptor, const Options& options)
    : FieldGenerator(options), descriptor_(descriptor) {
  SetPrimitiveVariables(descriptor, &variables_, options);

  if (descriptor->is_packed()) {
    variables_["packed_reader"] = "ReadPackedPrimitive";
    variables_["repeated_reader"] = "ReadRepeatedPrimitiveNoInline";
  } else {
    variables_["packed_reader"] = "ReadPackedPrimitiveNoInline";
    variables_["repeated_reader"] = "ReadRepeatedPrimitive";
  }
}

RepeatedPrimitiveFieldGenerator::~RepeatedPrimitiveFieldGenerator() {}

void RepeatedPrimitiveFieldGenerator::
GeneratePrivateMembers(io::Printer* printer) const {
  printer->Print(variables_,
    "EA::TDF::TdfPrimitiveVector< $value_tdf_type$ > m$name$;\n");
  if (descriptor_->is_packed() &&
      HasGeneratedMethods(descriptor_->file(), options_)) {
    printer->Print(variables_,
      "mutable int m$name$_ProtobufCachedSize = 0;\n");
  }
}

void RepeatedPrimitiveFieldGenerator::
GenerateAccessorDeclarations(io::Printer* printer) const {
  
    printer->Print(variables_,
        "$deprecated_attr$EA::TDF::TdfPrimitiveVector< $value_tdf_type$ >& get$name$();\n"
        "$deprecated_attr$const EA::TDF::TdfPrimitiveVector< $value_tdf_type$ >& get$name$() const;\n"
        "$deprecated_attr$void set$name$(const EA::TDF::TdfPrimitiveVector< $value_tdf_type$ >&);\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateInlineAccessorDefinitions(io::Printer* printer, bool is_inline) const {
    std::map<string, string> variables(variables_);
  variables["inline"] = is_inline ? "inline " : "";
  
  printer->Print(variables,
    "$inline$ const EA::TDF::TdfPrimitiveVector< $value_tdf_type$ >&\n"
    "$classname$::get$name$() const {\n"
    "  // @@protoc_insertion_point(field_list:$full_name$)\n"
    "  return m$name$;\n"
    "}\n"
    "$inline$ EA::TDF::TdfPrimitiveVector< $value_tdf_type$ >&\n"
    "$classname$::get$name$() {\n"
    "  // @@protoc_insertion_point(field_mutable_list:$full_name$)\n"
    "  return m$name$;\n"
    "}\n"
    "$inline$ void\n"
    "$classname$::set$name$(const EA::TDF::TdfPrimitiveVector< $value_tdf_type$ >& value) {\n"
    "  // @@protoc_insertion_point(field_mutable_list:$full_name$)\n"
    "  value.copyInto(m$name$);\n"
    "}\n"
    );
}

void RepeatedPrimitiveFieldGenerator::
GenerateClearingCode(io::Printer* printer) const {
  printer->Print(variables_, "get$name$().clearVector();\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  printer->Print(variables_, "$name$.MergeFrom(from.$name$);\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateSwappingCode(io::Printer* printer) const {
  printer->Print(variables_, "$name$.UnsafeArenaSwap(&other->$name$);\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateConstructorCode(io::Printer* printer) const {
  // Not needed for repeated fields.
}

void RepeatedPrimitiveFieldGenerator::
GenerateMergeFromCodedStream(io::Printer* printer) const {
  printer->Print(variables_,
      "EA_ASSERT_MSG(false, \"Non-packed types are not supported\");\n"
    "goto handle_unusual;\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateMergeFromCodedStreamWithPacking(io::Printer* printer) const {
  
  printer->Print(variables_,
    "DO_((::Blaze::protobuf::internal::WireFormatLite::$packed_reader$<\n"
    "         $type$, $wire_format_field_type$>(\n"
    "       input, get$name$())));\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateSerializeWithCachedSizes(io::Printer* printer) const {

  if (descriptor_->is_packed()) {
    // Write the tag and the size.
    printer->Print(variables_,
        "if (get$name$().size() > 0) {\n"
      "  ::google::protobuf::internal::WireFormatLite::WriteTag("
          "$number$, "
          "::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED, "
          "output);\n"
      "  output->WriteVarint32(m$name$_ProtobufCachedSize);\n"
      "}\n");
  }
  printer->Print(variables_,
      "for (int i = 0; i < get$name$().size(); i++) {\n");
  if (descriptor_->is_packed()) {
    printer->Print(variables_,
      "  ::google::protobuf::internal::WireFormatLite::Write$declared_type$NoTag(\n"
      "    get$name$()[i], output);\n");
  } else {
      printer->Print(variables_,
          "EA_ASSERT_MSG(false, \"Non-packed types are not supported\");\n");
  }
  printer->Print("}\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateSerializeWithCachedSizesToArray(io::Printer* printer) const {

  if (descriptor_->is_packed()) {
    // Write the tag and the size.
    printer->Print(variables_,
        "if (get$name$().size() > 0) {\n"
      "  target = ::google::protobuf::internal::WireFormatLite::WriteTagToArray(\n"
      "    $number$,\n"
      "    ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED,\n"
      "    target);\n"
      "  target = ::google::protobuf::io::CodedOutputStream::WriteVarint32ToArray(\n"
      "    m$name$_ProtobufCachedSize, target);\n"
      "}\n");
  }
  printer->Print(variables_,
      "for (int i = 0; i < get$name$().size(); i++) {\n");
  if (descriptor_->is_packed()) {
    printer->Print(variables_,
      "  target = ::google::protobuf::internal::WireFormatLite::\n"
      "    Write$declared_type$NoTagToArray(get$name$()[i], target);\n");
  } else {
      printer->Print(variables_,
          "EA_ASSERT_MSG(false, \"Non-packed types are not supported\");\n");
  }
  printer->Print("}\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateByteSize(io::Printer* printer) const {

  printer->Print(variables_,
    "{\n"
    "  int data_size = 0;\n");
  printer->Indent();
  int fixed_size = FixedSize(descriptor_->type());
  if (fixed_size == -1) {
    printer->Print(variables_,
        "for (int i = 0; i <get$name$().size(); i++) {\n"
      "  data_size += ::google::protobuf::internal::WireFormatLite::\n"
        "    $declared_type$Size(get$name$()[i]);\n"
      "}\n");
  } else {
    printer->Print(variables_,
      "data_size = $fixed_size$ * get$name$().size();\n");
  }

  if (descriptor_->is_packed()) {
    printer->Print(variables_,
      "if (data_size > 0) {\n"
      "  total_size += $tag_size$ +\n"
      "    ::google::protobuf::internal::WireFormatLite::Int32Size(data_size);\n"
      "}\n"
      "GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();\n"
      "m$name$_ProtobufCachedSize = data_size;\n"
      "GOOGLE_SAFE_CONCURRENT_WRITES_END();\n"
      "total_size += data_size;\n");
  } else {
      printer->Print(variables_,
          "EA_ASSERT_MSG(false, \"Non-packed types are not supported\");\n");
  }
  printer->Outdent();
  printer->Print("}\n");
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
