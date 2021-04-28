package com.ea.bridgedocgen.html

import com.ea.bridgedocgen.apt
import com.ea.bridgedocgen.jstype
import com.ea.bridgedocgen.SemanticException
import scala.xml.NodeSeq

class LinkBuilder(searchModules : Seq[apt.DefinitionModule], directoryDepth : Int) {
  private case class ResolvedNamedDefinition(
    inteface : apt.DefinitionModule, 
    namedDefinition : apt.NamedDefinition
  ) 

  private def resolveNamedDefinition(name : String) : Option[ResolvedNamedDefinition] = {
    // Find an module with the matching definition
    val resolutions = for(module <- searchModules; 
                          definition <- module.namedDefinitions;
                          if definition.name == name)
                      yield
                         ResolvedNamedDefinition(module, definition)
    
    resolutions.headOption
  }

  // Creates a nested linker
  def nestedLinker = new LinkBuilder(searchModules, directoryDepth + 1)
   
  // Static path
  def urlFor(path : String) : String= ("../" * directoryDepth) + path

  // Modules
  def urlFor(module : apt.DefinitionModule) : String =
    urlFor(module.name + "/index.xhtml")

  def linkTo(module : apt.DefinitionModule) : NodeSeq = 
    <a href={ urlFor(module) }>{ module.name }</a>
  
  // Named definitions
  def urlFor(module : apt.DefinitionModule, namedDefinition : apt.NamedDefinition) : String = {
    urlFor(module.name + "/" + namedDefinition.name + ".xhtml")
  }

  def linkTo(module : apt.DefinitionModule, namedDefinition : apt.NamedDefinition) : NodeSeq = 
    <a href={ urlFor(module, namedDefinition) }>{ namedDefinition.name }</a>

  // Named definition children
  def urlFor(module : apt.DefinitionModule, namedDefinition : apt.NamedDefinition, child : apt.Documentable) : String = {
    urlFor(module, namedDefinition) + "#" + child.name
  }

  def linkToWidl(module : apt.DefinitionModule) =
    <a class="widl" href={ urlFor(module.name + "/" + module.name + "Definitions.widl") }>View as Web IDL</a> 
  
  def linkToWidl(module : apt.DefinitionModule, namedDefinition : apt.NamedDefinition) =
    <a class="widl" href={ urlFor(module.name + "/" + namedDefinition.name + ".widl") }>View as Web IDL</a> 
  
  // No child linkTo as we can't tell how qualified the name should be with the context information we're given

  // JavaScript Types
  def urlFor(jsType : jstype.JavaScriptType) : String = jsType match {
    case intrinsic : jstype.IntrinsicType => 
      "https://developer.mozilla.org/en/JavaScript/Reference/Global_Objects/" + intrinsic.mdcGlobalName
    case jstype.CustomType(name) =>
      resolveNamedDefinition(name) match {
        case Some(ResolvedNamedDefinition(module, namedDefinition)) =>
          urlFor(module, namedDefinition)
        case None =>
          throw new SemanticException("Tried to link to unknown type " + name)
      }
    case _ => 
      throw new SemanticException("Cannot create URLs for complex types")
  }

  def linkTo(jsType : jstype.JavaScriptType) : NodeSeq = jsType match {
    case jstype.UnionType(memberTypes) =>
      xml.Text("(") ++
        memberTypes.map(linkTo(_)).reduce(_ ++ xml.Text(" or ") ++ _) ++
        xml.Text(")")
    case jstype.NullableType(innerType) => 
      linkTo(innerType) ++ xml.Text("?")
    case jstype.ArrayType(elementType) => 
      linkTo(elementType) ++ xml.Text("[]")
    case _ =>
      <a href={ urlFor(jsType) }>{ jsType.typeName }</a>
    }
}
