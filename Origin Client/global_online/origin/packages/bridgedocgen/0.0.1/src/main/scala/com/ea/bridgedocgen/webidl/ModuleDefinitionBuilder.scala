package com.ea.bridgedocgen.webidl

import collection.mutable.ListBuffer
import com.ea.bridgedocgen.apt

object ModuleDefinitionBuilder {
  def apply(module : apt.DefinitionModule) : String = {
    val sortedInterfaces = module.interfaces.sortBy(_.name)
    val sortedEnums = module.enums.sortBy(_.name)
    val sortedDicts = module.dicts.sortBy(_.name)
    val sortedTypedefs = module.typedefs.sortBy(_.name)
 
    val allDeclarations = sortedEnums.map(EnumDefinitionBuilder(_)) ++
                          sortedDicts.map(DictDefinitionBuilder(_)) ++
                          sortedTypedefs.map(TypedefDefinitionBuilder(_)) ++
                          sortedInterfaces.map(InterfaceDefinitionBuilder(_))

    module match {
      case featureModule : apt.FeatureModule =>
        val builder = new ListBuffer[String]()

        // Make Window implement our interface
        val windowInterfaceName = module.name.capitalize + "Feature"

        builder += "[NoInterfaceObject]"
        builder += "interface " + windowInterfaceName + " {"
        builder += "\t" + "readonly attribute " + featureModule.rootInterface.name + " " + module.name + ";"
        builder += "};"
        builder += ""
        builder += "Window implements " + windowInterfaceName + ";"

        allDeclarations.mkString("\r\n") + "\r\n" + builder.mkString("\r\n")
      case _ =>
        allDeclarations.mkString("\r\n")
    }

  }
}
