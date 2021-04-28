package com.ea.bridgedocgen

object JavaScriptValueParser {
  def apply(parseAsType : jstype.JavaScriptType)(input : String) : jsvalue.JavaScriptValue = parseAsType match {
    // XXX: Assuming custom types are enums
    case jstype.StringType | jstype.CustomType(_) =>
      jsvalue.StringValue(input)

    case jstype.NullableType(innerType) =>
      input match {
        case "null" =>
          jsvalue.NullValue
        case _ =>
          // Recurse
          JavaScriptValueParser(innerType)(input)
      }

    case jstype.IntegerType => 
      try {
        jsvalue.IntegerValue(input.toLong)
      }
      catch {
        case _ : NumberFormatException =>
          throw new ParseErrorException("Unable to parse '" + input + "' as integer")
      }
    
    case jstype.FloatType => 
      try {
        jsvalue.FloatValue(input.toDouble)
      }
      catch {
        case _ : NumberFormatException =>
          throw new ParseErrorException("Unable to parse '" + input + "' as float")
      }
    
    case jstype.BooleanType => 
      input match {
        case "true" =>
          jsvalue.BooleanValue(true)
        case "false" =>
          jsvalue.BooleanValue(false)
        case _ =>
          throw new ParseErrorException("Unable to parse '" + input + "' as boolean")
      }

    case _ =>
      throw new ParseErrorException("Can't parse value literals for " + parseAsType.toString)
  }
}
