package com.ea.bridgedocgen
import scala.xml._

object DictParser {
  import NodeSeqMixins._
  
  def apply(name : String, root : NodeSeq) : apt.Dict = {
    val members : Seq[apt.DictMember] = root \ "member" map { memberNode =>
      apt.DictMember(memberNode.name, memberNode.description, memberNode.jsType)
    }

    val inherits = root \ "@inherits" match {
      case Seq() => None
      case value @ _ => Some(value.toString)
    }

    apt.Dict(
      name=name, 
      inherits=inherits,
      description=root.description,
      members=members)
  }
}
