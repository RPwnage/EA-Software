package com.ea.bridgedocgen

import scala.xml._

object InterfaceParser {
  import NodeSeqMixins._

  def apply(name : String, root : NodeSeq) : apt.Interface = {
    val properties : Seq[apt.Property] = root \ "property" map { propertyNode =>
      val readOnly = propertyNode \ "@readonly" match {
        case Seq() => false
        case value @ _ => value.toString == "true"
      }
      
      val inherit = propertyNode \ "@inherit" match {
        case Seq() => false
        case value @ _ => value.toString == "true"
      }

      apt.Property(propertyNode.name, propertyNode.description, propertyNode.jsType, readOnly, inherit) 
    }

    val signals : Seq[apt.Signal] = root \ "signal" map { signalNode =>
      apt.Signal(signalNode.name, signalNode.description, signalNode.parameters)
    }

    val methods : Seq[apt.Method] = root \ "method" map { methodNode =>
      val returnType = methodNode \ "@return" match {
        case Seq() => None
        case nonEmpty @ _ => JavaScriptTypeParser(nonEmpty.toString) match {
          case JavaScriptTypeParser.Success(parsedType, _) =>
            Some(parsedType)
          case err =>
            throw new ParseErrorException("Unable to parse JavaScript type: " + err.toString)
        }
      }

      apt.Method(methodNode.name, methodNode.description, methodNode.parameters, returnType)
    }

    val moduleRoot = root \ "@root" match {
      case Seq() => false
      case value @ _ => value.toString == "true"
    }

    val inherits = root \ "@inherits" match {
      case Seq() => None
      case value @ _ => Some(value.toString)
    }

    apt.Interface(name=name, 
                  inherits=inherits, 
                  description=root.description, 
                  methods=methods, 
                  signals=signals, 
                  properties=properties, 
                  moduleRoot=moduleRoot)
  }
}
