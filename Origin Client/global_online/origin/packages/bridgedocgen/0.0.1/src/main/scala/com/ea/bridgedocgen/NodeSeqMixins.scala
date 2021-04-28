package com.ea.bridgedocgen

import scala.xml._
import jstype.JavaScriptType

object NodeSeqMixins {
  implicit class Mixins(seq : NodeSeq) {
    // Used for required nodes and parameters
    def \!(that : String) : NodeSeq = {
      val returnSeq : NodeSeq = seq \ that

      returnSeq match {
        case Seq() => throw new ParseErrorException("Unable to evaluate " + that + " for " + seq.toString)
        case _ => returnSeq
      }
    }

    def description : Option[String] = seq \ "description" match {
      case Seq(<description>{ inner @ _* }</description>) => Some(inner.text)
      case Seq() => None
    }

    def name = (this \! "@name").toString

    def jsType : JavaScriptType = JavaScriptTypeParser((this \! "@type").toString) match {
        case JavaScriptTypeParser.Success(parsedType, _) => parsedType
        case err =>
          throw new ParseErrorException("Unable to parse JavaScript type: " + err.toString)
      }

    def parameters = seq \ "param" map { paramNode =>
      val paramType = paramNode.jsType
    
      val defaultValue = (paramNode \ "@default") match {
        case Seq(defaultAttr) =>
          // Parse the JavaScript type
          Some(JavaScriptValueParser(paramType)(defaultAttr.toString))
        case _ =>
          None
      }
      
      val optional = defaultValue match {
        // Default values implicitly make parameters optional
        case Some(_) => true
        case None =>
          paramNode \ "@optional" match {
            case Seq() => false
            case value @ _ => value.toString == "true"
          }
      }

      apt.Parameter(paramNode.name, paramNode.description, paramType, defaultValue, optional)
    }
  }
}
