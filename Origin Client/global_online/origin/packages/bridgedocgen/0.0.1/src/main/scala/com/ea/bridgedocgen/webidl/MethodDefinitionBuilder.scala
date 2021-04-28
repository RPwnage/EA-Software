package com.ea.bridgedocgen.webidl

import com.ea.bridgedocgen.jstype.JavaScriptType
import com.ea.bridgedocgen.apt

object MethodDefinitionBuilder {
  def apply(method : apt.Method) = {
    val returnString = method.returnType match {
      case Some(jsType) => jsType.webidlString
      case None => "void"
    }

    val argumentStrings = method.parameters.map(ParameterDefinitionBuilder(_))
    returnString + " " + method.name + "(" + argumentStrings.mkString(", ") + ");"
  }
}
