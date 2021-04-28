package com.ea.bridgedocgen.html

import com.ea.bridgedocgen.apt
import scala.xml.NodeSeq
import scala.xml.Text

object EnumDetailBuilder {
  def apply(module : apt.DefinitionModule, enum : apt.Enum)(implicit linker : LinkBuilder) = {
    val describer = new DocumentableDescriber(module, enum, linker) 

    def valueRow(value : apt.EnumValue) = 
      <tr>
        <td>"{value.name}"</td>
        <td>
          { describer.describeInline(value) }
        </td>
      </tr>

    val pageTitle = enum.name  + " Enumeration Reference"

    val sortedValues = enum.values.sortBy(_.name)

    val pageNodes = <h1>{pageTitle}</h1> ++ describer.describe(enum) ++
      <table class="enumeration">
        <tr>
          <th>String Value</th>
          <th>Description</th>
        </tr>
        { sortedValues map valueRow }
      </table>

    HTMLDocumentation(pageTitle, 
      describer.describe(enum) ++
      <table class="enumeration">
        <tr>
          <th>String Value</th>
          <th>Description</th>
        </tr>
        { sortedValues map valueRow }
      </table>
    )
  }
}
