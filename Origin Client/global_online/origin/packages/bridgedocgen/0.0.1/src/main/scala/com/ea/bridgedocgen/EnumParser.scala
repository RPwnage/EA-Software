package com.ea.bridgedocgen
import scala.xml._

object EnumParser {
  import NodeSeqMixins._

  def apply(name : String, root : NodeSeq) : apt.Enum = {
    val values : Seq[apt.EnumValue] = root \ "value" map { valueNode =>
      apt.EnumValue(valueNode.name, valueNode.description)
    }

    apt.Enum(name, root.description, values)
  }
}
