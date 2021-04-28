package com.ea.bridgedocgen.webidl

import com.ea.bridgedocgen.apt
import collection.mutable.ListBuffer

object InterfaceDefinitionBuilder {
  private def signalConnectorDeclaration(signalConnectorName : String, signal : apt.Signal) = {
    val builder = new ListBuffer[String]()
    val slotTypeName = signalConnectorName + "Slot"

    val parameterStrings = signal.parameters.map(ParameterDefinitionBuilder(_))

    // Make our callback function
    builder += "callback " + slotTypeName + " = void (" + parameterStrings.mkString(", ") + ");"
    builder += ""

    builder += "[NoInterfaceObject]"
    builder += "interface " + signalConnectorName + " {"
    builder += "\t" + "void connect(" + slotTypeName + " function);";
    builder += "\t" + "void disconnect(" + slotTypeName + " function);";
    builder += "};"
    builder += ""

    builder
  }

  def apply(interface : apt.Interface) : String = {
    val builder = new ListBuffer[String]()

    // Declare our signal connector
    val declaredSignalConnectors = interface.signals.sortBy(_.name) map { signal =>
      val signalConnectorName = interface.name + signal.name.capitalize + "Connector"

      builder ++= signalConnectorDeclaration(signalConnectorName, signal)

      (signal, signalConnectorName)
    }
    
    // Start the actual interface
    val inheritanceDef = interface.inherits match {
      case Some(inheritsName) => " : " + inheritsName
      case None => ""
    }
    
    builder += "[NoInterfaceObject]"
    builder += "interface " + interface.name + inheritanceDef + " {"

    // Declare all our properties (attributes in WebIDL)
    for(property <- interface.properties.sortBy(_.name)) {
      val attributeKeyword = if (property.readOnly) {
        "readonly attribute"
      }
      else if (property.inherit) {
        "inherit attribute"
      }
      else {
        "attribute"
      }

      builder += "\t" + attributeKeyword + " " + property.jsType.webidlString + " " + property.name + ";"
    }

    // Declare all of our methods (operations in WebIDL)
    for(method <- interface.methods.sortBy(_.name)) {
      builder += "\t" + MethodDefinitionBuilder(method)
    }

    // Declare our signal connector attributes
    for((signal, signalConnectorName) <- declaredSignalConnectors) {
      builder += "\t" + "readonly attribute " + signalConnectorName + " " + signal.name + ";"
    }

    // Close the interface
    builder += "};"
    builder += ""

    builder.mkString("\r\n")
  }

}
