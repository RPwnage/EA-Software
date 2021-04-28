package com.ea.bridgedocgen.html

import com.ea.bridgedocgen.apt
import com.ea.bridgedocgen.jstype
import scala.xml.NodeSeq
import scala.xml.Text

object TypedefDetailBuilder {
  def apply(module : apt.DefinitionModule, typedef : apt.Typedef)(implicit linker : LinkBuilder) = {
    val describer = new DocumentableDescriber(module, typedef, linker) 

    def definitionDetail() =
      <section class="member-detail typedef-definition">
        <h3>Type Definition</h3>
        <p class="description">
          { linker.linkTo(typedef.jsType) }
        </p>
      </section>

    val pageTitle = typedef.name  + " Typedef Reference"

    HTMLDocumentation(pageTitle,
      describer.describe(typedef) ++ definitionDetail
    )
  }
}
