package com.ea.bridgedocgen.apt

import com.ea.bridgedocgen.jstype.JavaScriptType
import com.ea.bridgedocgen.jsvalue.JavaScriptValue
import com.ea.bridgedocgen.SemanticException

sealed trait Documentable {
  def name : String
  def description : Option[String]

  def children : Seq[Documentable] = Seq()
}

sealed trait MethodLike extends Documentable {
  def parameters : Seq[Parameter]
  override def children = parameters
}

// This name is borrowed from Web IDL. It doesn't quite make sense to me
sealed abstract class NamedDefinition extends Documentable 

sealed trait DefinitionModule {
  def interfaces : Seq[Interface]
  def enums : Seq[Enum]
  def dicts : Seq[Dict]
  def typedefs : Seq[Typedef]
  
  def name : String

  def namedDefinitions : Seq[NamedDefinition] = interfaces ++ enums ++ dicts ++ typedefs
}

sealed trait Inheritable {
  def inherits : Option[String]
}

case class Parameter(name : String, description : Option[String], jsType : JavaScriptType, defaultValue : Option[JavaScriptValue], optional : Boolean) extends Documentable

case class Signal(name : String, description : Option[String], parameters : Seq[Parameter]) extends MethodLike 

case class Method(name : String, description : Option[String], parameters : Seq[Parameter], returnType : Option[JavaScriptType]) extends MethodLike

case class Property(name : String, description : Option[String], jsType : JavaScriptType, readOnly : Boolean, inherit : Boolean) extends Documentable 

case class Interface(name : String, inherits : Option[String], description : Option[String], methods : Seq[Method], signals : Seq[Signal], properties : Seq[Property], moduleRoot : Boolean) extends NamedDefinition with Inheritable {
  override def children = methods ++ signals ++ properties
}

case class Enum(name : String, description : Option[String], values : Seq[EnumValue]) extends NamedDefinition {
  override def children = values 
}

case class EnumValue(name : String, description : Option[String]) extends Documentable

case class Dict(name : String, inherits : Option[String], description : Option[String], members : Seq[DictMember]) extends NamedDefinition with Inheritable

case class DictMember(name : String, description : Option[String], jsType : JavaScriptType) extends Documentable 

case class Typedef(name : String, description : Option[String], jsType : JavaScriptType) extends NamedDefinition

case class FeatureModule(name : String, description : Option[String], interfaces : Seq[Interface], enums : Seq[Enum], dicts : Seq[Dict], typedefs : Seq[Typedef]) extends Documentable with DefinitionModule {
  // This should be overridable in the future
  def featureUri = "http://widgets.dm.origin.com/features/interfaces/" + name

  def rootInterface = interfaces.filter(_.moduleRoot).toList match {
    case interface :: Nil =>
      interface
    
    case Nil => 
      throw new SemanticException(s"No root interface specified for ${name}")

    case multipleInterfaces =>
      throw new SemanticException(s"Multiple root interfaces specified for ${name}: "  + multipleInterfaces.map(_.name).mkString(", "))
  }

  override def children = namedDefinitions
}

case class CommonModule(interfaces : Seq[Interface], enums : Seq[Enum], dicts : Seq[Dict], typedefs : Seq[Typedef]) extends DefinitionModule {
  override def name = "common"
}

case class Definitions(common : Option[CommonModule], featureModules : Seq[FeatureModule]) {
  def modules : Seq[DefinitionModule] = common.toList ++ featureModules
}
