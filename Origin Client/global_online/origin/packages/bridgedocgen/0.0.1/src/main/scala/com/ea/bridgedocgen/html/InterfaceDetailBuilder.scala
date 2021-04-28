package com.ea.bridgedocgen.html

import scala.xml.NodeSeq
import scala.xml.Text

import com.ea.bridgedocgen.apt
import com.ea.bridgedocgen.jstype

object InterfaceDetailBuilder {
  def apply(module : apt.DefinitionModule, interface : apt.Interface)(implicit linker : LinkBuilder) = {
    val describer = new DocumentableDescriber(module, interface, linker) 

    def signature(parameters : Seq[apt.Parameter]) = 
      <span class="signature">(
        { 
          if (!parameters.isEmpty) {
            parameters.map(parameterSummary).reduceLeft((a, b) => 
              (a :+ xml.Text(", ")) ++ b
            )
          }
        }
      )</span>

    def parameterSummary(parameter : apt.Parameter) : NodeSeq = 
      <span class="parameter">
        { linker.linkTo(parameter.jsType) } { parameter.name }
      </span>

    def signatureDetail(parameters : Seq[apt.Parameter]) =
      if (parameters.isEmpty) {
        NodeSeq.Empty
      }
      else {
        <section class="signature-detail">
          <h4>Parameters</h4>
          <dl>
            { parameters.map(parameterDetail) }
          </dl>
        </section>
      }

    def parameterDetail(parameter : apt.Parameter) = 
      <dt>
        { parameterSignature(parameter) }
      </dt> ++ 
      <dd>
        { describer.describe(parameter) }
      </dd>

    def parameterSignature(parameter : apt.Parameter) : NodeSeq = parameter.optional match {
      case true =>
        parameter.defaultValue match {
          case Some(defaultValue) =>
            // Show the default
            xml.Text(parameter.name + " : ") ++ 
              linker.linkTo(parameter.jsType) ++
              xml.Text(" = " + defaultValue.webidlString) 
          case None => 
            // Just show an optional indicator
            xml.Text(parameter.name + " : ") ++ linker.linkTo(parameter.jsType) ++ <span class="optional"> [optional]</span>
        }
      case false =>
        xml.Text(parameter.name + " : ") ++ linker.linkTo(parameter.jsType)
    }

    def propertySection(properties : Seq[apt.Property]) =
      if (properties.isEmpty) {
        NodeSeq.Empty
      } else {
        <section class="members properties">
          <h2>Properties</h2>
          { properties map propertyDetail }
        </section>
      }

    def propertyDetail(property : apt.Property) = 
      <section class={ propertyClasses(property) }>
        <a name={ property.name }/>
        <h3>{property.name } : { linker.linkTo(property.jsType) }</h3>
        { describer.describe(property) }
      </section>


    def propertyClasses(property : apt.Property) = {
      val baseClasses = "member-detail property-detail"

      if (property.readOnly) {
        baseClasses + " readonly"
      }
      else {
        baseClasses
      }
    }

    def methodSection(methods : Seq[apt.Method]) =
      if (methods.isEmpty) {
        NodeSeq.Empty
      } else {
        <section class="members methods">
          <h2>Methods</h2>
          { methods map methodDetail }
        </section>
      }

    def methodDetail(method : apt.Method) = 
      <section class="member-detail method-detail">
        <a name={ method.name }/>
        <h3>
          { methodReturnDetail(method.returnType) }
          { method.name }
          { signature(method.parameters) } 
        </h3>
        { signatureDetail(method.parameters) }
        { describer.describe(method) }
      </section>


    def methodReturnDetail(returnType : Option[jstype.JavaScriptType]) = returnType match {
      case None => xml.Text("void")
      case Some(jsType) => linker.linkTo(jsType)
    }

    def signalSection(signals : Seq[apt.Signal]) =
      if (signals.isEmpty) {
        NodeSeq.Empty
      } else {
        <section class="members signals">
          <h2>Signals</h2>
          { signals map signalDetail }
        </section>
      }

    def signalDetail(signal : apt.Signal) = 
      <section class="member-detail signal-detail">
        <a name={ signal.name }/>
        <h3>
          { signal.name }
          { signature(signal.parameters) }
        </h3>
        { signatureDetail(signal.parameters) }
        { describer.describe(signal) }
      </section>

    def inheritsSection(inherits : Option[String]) = inherits match {
      case None => NodeSeq.Empty
      case Some(inheritsName) =>
        <section class="members inherits">
          <h2>Inherits</h2>
          { linker.linkTo(jstype.CustomType(inheritsName)) }
        </section>
    }
    
    def derivedSection(derived : Seq[apt.Interface]) =
      if (derived.isEmpty) { 
        NodeSeq.Empty
      }
      else {
        <section class="members derived">
          <h2>Derived Interfaces</h2>
          {
            derived.map(linker.linkTo(module, _)).reduceLeft((a, b) => 
              (a :+ xml.Text(", ")) ++ b
            )
          }
        </section>
      }

    def instanceInformation = module match {
      case _ : apt.CommonModule =>
        <p class="instance-info">
          This is a common interface that can only be accessed through other features.
          This interface has no JavaScript constructor and cannot be instantiated directly.
        </p>
      case featureModule : apt.FeatureModule =>
        interface.moduleRoot match {
          case true =>
            <p class="instance-info">
              This interface has a global instance accessible as <code class="javascript">window.{featureModule.name}</code>.
              This interface has no JavaScript constructor and cannot be instantiated directly.
            </p>
          case false =>
            <p class="instance-info">
              This interface can only be accessed through {linker.linkTo(featureModule, featureModule.rootInterface)} or one of its subobjects.
              This interface has no JavaScript constructor and cannot be instantiated directly.
            </p>
        }
    }

    val pageTitle = interface.name  + " Interface Reference"

    val sortedProperties = interface.properties.sortBy(_.name)
    val sortedMethods = interface.methods.sortBy(_.name)
    val sortedSignals = interface.signals.sortBy(_.name)

    val derivedInterfaces = module.interfaces.filter(_.inherits == Some(interface.name)).sortBy(_.name)

    val memberSections = 
      inheritsSection(interface.inherits) ++ 
      derivedSection(derivedInterfaces) ++ 
      propertySection(sortedProperties) ++ 
      methodSection(sortedMethods) ++
      signalSection(sortedSignals)

    HTMLDocumentation(pageTitle, 
      describer.describe(interface) ++ instanceInformation ++ memberSections
    )
  }
}
