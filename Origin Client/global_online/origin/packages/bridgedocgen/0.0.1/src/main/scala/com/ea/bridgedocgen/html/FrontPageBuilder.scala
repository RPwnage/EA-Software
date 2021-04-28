package com.ea.bridgedocgen.html

import com.ea.bridgedocgen.apt
import scala.xml.NodeSeq

object FrontPageBuilder {
  def apply(definitions : apt.Definitions)(implicit linker : LinkBuilder) = {
    val sortedFeatureModules = definitions.featureModules.sortBy(_.name)

    def featureRow(feature : apt.FeatureModule) : NodeSeq = {
      <tr>
        <td>{ linker.linkTo(feature) }</td>
        <td>{ linker.linkTo(feature, feature.rootInterface) }</td>
        <td>{ feature.description.getOrElse("") }</td>
      </tr>
    }

    val commonDefinitions = definitions.common match {
      case Some(module) => ModuleDetailBuilder(module).content
      case None => NodeSeq.Empty
    }

    HTMLDocumentation("JavaScript Bridge Documentation", 
      <h2>Feature Modules</h2>
      <table class="modules">
        <tr>
          <th>Feature</th><th>Root Object</th><th>Description</th>
        </tr>
        { sortedFeatureModules map featureRow  }
      </table> ++
      commonDefinitions
    )
  }
}
