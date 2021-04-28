package com.ea.bridgedocgen.html

import com.ea.bridgedocgen.apt
import com.ea.bridgedocgen.jstype
import scala.xml.NodeSeq
import scala.xml.Text

object DictDetailBuilder {
  def apply(module : apt.DefinitionModule, dict : apt.Dict)(implicit linker : LinkBuilder) = {
    val describer = new DocumentableDescriber(module, dict, linker) 

    def memberSection(members : Seq[apt.DictMember]) =
      <section class="members dict-members">
        <h2>Members</h2>
        { members map memberDetail }
      </section>

    def memberDetail(member : apt.DictMember) = 
      <section class="member-detail dict-member-detail">
        <a name={ member.name }/>
        <h3>{member.name } : { linker.linkTo(member.jsType) }</h3>
        { describer.describe(member) }
      </section>
    
    def inheritsSection(inherits : Option[String]) = inherits match {
      case None => NodeSeq.Empty
      case Some(inheritsName) =>
        <section class="members inherits">
          <h2>Inherits</h2>
          { linker.linkTo(jstype.CustomType(inheritsName)) }
        </section>
    }
    
    def derivedSection(derived : Seq[apt.Dict]) =
      if (derived.isEmpty) { 
        NodeSeq.Empty
      }
      else {
        <section class="members derived">
          <h2>Derived Dictionaries</h2>
          {
            derived.map(linker.linkTo(module, _)).reduceLeft((a, b) => 
              (a :+ xml.Text(", ")) ++ b
            )
          }
        </section>
      }


    val pageTitle = dict.name  + " Dictionary Reference"
    val sortedMembers = dict.members.sortBy(_.name)
    
    val derivedDicts = module.dicts.filter(_.inherits == Some(dict.name)).sortBy(_.name)

    HTMLDocumentation(pageTitle,
      describer.describe(dict) ++
      inheritsSection(dict.inherits) ++
      derivedSection(derivedDicts) ++
      memberSection(sortedMembers)
    )
  }
}
