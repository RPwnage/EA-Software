package com.ea.bridgedocgen.webidl

import com.ea.bridgedocgen.apt
  
object TypedefDefinitionBuilder {
  def apply(typedef : apt.Typedef) : String = {
    "typedef " + typedef.jsType.webidlString + " " + typedef.name + ";\r\n"
  }
}
