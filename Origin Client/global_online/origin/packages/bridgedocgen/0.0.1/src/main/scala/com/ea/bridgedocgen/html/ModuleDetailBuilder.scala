package com.ea.bridgedocgen.html

import com.ea.bridgedocgen.apt
import scala.xml.NodeSeq

object ModuleDetailBuilder {
  def apply(module : apt.DefinitionModule)(implicit linker : LinkBuilder) = {
    // Prefix each definition with "Common" for our common definitions
    def tableTitle(plural : String) = {
      module match {
        case (_ : apt.CommonModule) =>
          "Common " + plural
        case _ =>
          plural
      }
    }

    val descriptionHtml = module match {
      case apt.FeatureModule(_, Some(description), _, _, _, _) =>
        <p class="description">{description}</p>
      case _ =>
        NodeSeq.Empty
    }

    val widgetRequestHelp = module match {
      case featureModule : apt.FeatureModule =>
        <h2>Requesting from Web Widgets</h2>
        <p>
          Web widgets can request this feature by including
          <code>&lt;feature name="{featureModule.featureUri}" /&gt;</code>
          in their <code>config.xml</code>
        </p>
      case _ =>
        NodeSeq.Empty
    }

    def definitionTable(definitions : Seq[apt.NamedDefinition], singular : String, plural : String) : NodeSeq =
      if (definitions.isEmpty) {
        NodeSeq.Empty
      }
      else {
        <h2>{ tableTitle(plural) }</h2> ++
        <table class="definition-summary" >
          <tr>
            <th>{ singular } Name</th>
            <th>Description</th>
          </tr>
          { definitions map definitionRow }
        </table>
      }
    
    def definitionRow(definition : apt.NamedDefinition) : NodeSeq = 
      <tr>
        <td>{ linker.linkTo(module, definition) }</td>
        <td>{ definition.description.getOrElse(NodeSeq.Empty) }</td>
      </tr>

    val pageTitle = module.name + " Feature"
    val sortedInterfaces = module.interfaces.sortBy(_.name)
    val sortedEnums = module.enums.sortBy(_.name)
    val sortedDicts = module.dicts.sortBy(_.name)
    val sortedTypedefs = module.typedefs.sortBy(_.name)

    HTMLDocumentation(pageTitle, 
      descriptionHtml ++
      widgetRequestHelp ++ 
      definitionTable(sortedInterfaces, "Interface", "Interfaces") ++
      definitionTable(sortedEnums, "Enumeration", "Enumerations") ++
      definitionTable(sortedDicts, "Dictionary", "Dictionaries") ++
      definitionTable(sortedTypedefs, "Typedef", "Typedefs")
    )

  }
}
