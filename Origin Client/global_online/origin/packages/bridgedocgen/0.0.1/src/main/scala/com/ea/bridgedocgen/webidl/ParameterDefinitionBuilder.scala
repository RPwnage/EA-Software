package com.ea.bridgedocgen.webidl

import com.ea.bridgedocgen.jstype.JavaScriptType
import com.ea.bridgedocgen.apt

object ParameterDefinitionBuilder {
  def apply(parameter : apt.Parameter) = {
    (parameter.optional match {
      case true => "optional "
      case false => ""
    }) +
    parameter.jsType.webidlString + " " + parameter.name + 
    (parameter.defaultValue match {
      case Some(default) => " = " + default.webidlString 
      case None => ""
    })
  }
}
