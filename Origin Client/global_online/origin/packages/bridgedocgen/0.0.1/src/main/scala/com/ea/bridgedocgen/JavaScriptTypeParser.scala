package com.ea.bridgedocgen

import scala.util.parsing.combinator._
import scala.util.matching.Regex
import com.ea.bridgedocgen.jstype._

object JavaScriptTypeParser extends RegexParsers with PackratParsers {
  def apply(input : String) : ParseResult[JavaScriptType] =
    parseAll(jsType, input)

  lazy val jsType : PackratParser[JavaScriptType] =
    arrayType | nullableType | unionType | intrinsicType | customType
  
  lazy val intrinsicType = booleanType | floatType | integerType | objectType | stringType | dateType | anyType

  lazy val booleanType = "Boolean" ^^^ BooleanType
  lazy val dateType    = "Date"    ^^^ DateType
  lazy val integerType = "Integer" ^^^ IntegerType
  lazy val floatType   = "Float"   ^^^ FloatType
  lazy val stringType  = "String"  ^^^ StringType
  lazy val objectType  = "Object"  ^^^ ObjectType
  lazy val anyType     = "Any"     ^^^ AnyType

  lazy val nullableType = jsType <~ "?" ^^ { NullableType(_) }
  lazy val arrayType = jsType <~ "[]" ^^ { ArrayType(_) }

  lazy val unionType = "(" ~> rep1sep(jsType, "or") <~ ")" ^^ { UnionType(_) }

  lazy val customType  = identifier ^^ { CustomType(_) }
  lazy val identifier  = """[A-Za-z_]\w+""".r
}
