package com.ea.bridgedocgen.html

import com.ea.bridgedocgen.apt
import xml.NodeSeq

class DocumentableDescriber(module : apt.DefinitionModule, namedDefintion : apt.NamedDefinition, linker : LinkBuilder) {
  // Require at least one uppercase and one lowercase charactr
  private val autolinkRegexp = """([a-z]+[A-Z]+|[A-Z]+[a-z]+).*"""

  // Figure out which members are eligible for autolinking without being qualified
  // This is a heuristic to avoid linking eg every instance of "progress"
  private val autolinkableChildren = 
    namedDefintion.children.filter(_.name.matches(autolinkRegexp))

  // Build the links to the autolink unqualified children
  private val autolinkChildWords = (autolinkableChildren.map { child =>
    (child.name -> linker.urlFor(module, namedDefintion, child))
  }).toMap
  
  // Allow explicit links to methods and signals by appending ()
  private val methodLikeWords = (autolinkableChildren.flatMap { child =>
    child match {
      case _ : apt.MethodLike =>
        Some(child.name + "()" -> linker.urlFor(module, namedDefintion, child))
      case _ => 
        None
    }
  }).toMap

  // Build the qualified links (Definition.child)
  val qualifiedDefinitionWords = (module.namedDefinitions map { linkedDefinition =>
    linkedDefinition.children map { child =>
      (linkedDefinition.name + "." + child.name -> linker.urlFor(module, linkedDefinition, child))
    }
  }).flatten.toMap

  // Make our hyperlinker
  val allWords = autolinkChildWords ++ methodLikeWords ++ qualifiedDefinitionWords
  private val hyperlinker = HyperlinkText(allWords)_

  def describe(toDescribe : apt.Documentable) = toDescribe.description match {
      case Some(description) => <p class="description">{ describeInline(toDescribe) }</p>
      case None => NodeSeq.Empty
  }

  def describeInline(toDescribe : apt.Documentable) = toDescribe.description match {
      case Some(description) => hyperlinker(description)
      case None => NodeSeq.Empty
  }
}
