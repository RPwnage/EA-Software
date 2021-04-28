package com.ea.bridgedocgen.jstype

sealed abstract class JavaScriptType {
  def webidlString : String

  // Web IDL type name
  def typeName : String

}

sealed abstract class IntrinsicType extends JavaScriptType {
  // Name in the MDC docs
  def mdcGlobalName : String = typeName
}

sealed abstract class NumberType extends IntrinsicType {
  override def mdcGlobalName = "Number"
}

case class ArrayType(elementType : JavaScriptType) extends IntrinsicType {
  override def typeName = elementType.typeName + "[]"
  override def webidlString = elementType.webidlString + "[]"

  override def mdcGlobalName = "Array"
}

case class NullableType(innerType : JavaScriptType) extends JavaScriptType {
  override def typeName = innerType.typeName + "?"
  override def webidlString = innerType.webidlString + "?"
}

case object IntegerType extends NumberType  {
  override def typeName = "Integer"
  override def webidlString = "long long" 
}

case object FloatType extends NumberType  {
  override def typeName = "Float"
  override def webidlString = "double"
}

case object StringType extends IntrinsicType {
  override def typeName = "String"
  override def webidlString = "DOMString"
}

case object ObjectType extends IntrinsicType {
  override def typeName = "Object"
  override def webidlString = "Object"
}

case object BooleanType extends IntrinsicType {
  override def typeName = "Boolean"
  override def webidlString = "boolean"
}

case object DateType extends IntrinsicType {
  override def typeName = "Date"
  override def webidlString = "Date"
}

case object AnyType extends IntrinsicType {
  override def typeName = "Any"
  override def webidlString = "any"
}

case class UnionType(memberTypes : Seq[JavaScriptType]) extends JavaScriptType {
  def typeName = "(" + memberTypes.map(_.typeName).mkString(" or ") + ")"
  def webidlString = "(" + memberTypes.map(_.webidlString).mkString(" or ") + ")"
}

case class CustomType(name : String) extends JavaScriptType {
  override def typeName = name
  override def webidlString = name
}
