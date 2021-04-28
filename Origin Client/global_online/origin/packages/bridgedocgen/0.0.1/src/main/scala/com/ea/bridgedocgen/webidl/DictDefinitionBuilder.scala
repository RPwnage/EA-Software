package com.ea.bridgedocgen.webidl

import com.ea.bridgedocgen.apt
import collection.mutable.ListBuffer
  
object DictDefinitionBuilder {
  def apply(dict : apt.Dict) : String = {
    val builder = new ListBuffer[String]()

    val inheritanceDef = dict.inherits match {
      case Some(inheritsName) => " : " + inheritsName
      case None => ""
    }

    builder += "dictionary " + dict.name + inheritanceDef + " {"
    
    for(member <- dict.members) {
      builder += "\t" + member.jsType.webidlString + " " + member.name + ";"
    }

    builder += "};"
    builder += ""

    builder.mkString("\r\n")
  }
}
