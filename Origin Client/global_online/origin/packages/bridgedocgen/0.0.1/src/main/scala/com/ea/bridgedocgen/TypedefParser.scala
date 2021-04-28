package com.ea.bridgedocgen
import scala.xml._

object TypedefParser {
  import NodeSeqMixins._
  
  def apply(name : String, root : NodeSeq) : apt.Typedef = {
    apt.Typedef(name, root.description, root.jsType)
  }
}
