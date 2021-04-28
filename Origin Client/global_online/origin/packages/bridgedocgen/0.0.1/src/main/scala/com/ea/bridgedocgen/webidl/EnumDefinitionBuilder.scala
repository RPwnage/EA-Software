package com.ea.bridgedocgen.webidl

import com.ea.bridgedocgen.apt
import collection.mutable.ListBuffer
  
object EnumDefinitionBuilder {
  def apply(enum : apt.Enum) : String = {
    "enum " + enum.name + " { " + enum.values.map('"' + _.name + '"').mkString(", ") + " };\r\n"
  }
}
