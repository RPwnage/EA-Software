package test.scala

import com.ea.bridgedocgen._
import jstype._
import jsvalue._
import org.scalatest.Suite
import scala.xml

class InterfaceParserSuite extends Suite {
  def testEmptyInterface() {
    expectResult(apt.Interface(
      name="test",
      inherits=None,
      description=None, 
      methods=Nil, 
      signals=Nil, 
      properties=Nil, 
      moduleRoot=false))
    {
      InterfaceParser("test", <interface/>)
    }
  }
  
  def testRootInterface() {
    expectResult(apt.Interface(
      name="test",
      inherits=None,
      description=None,
      methods=Nil,
      signals=Nil, 
      properties=Nil, 
      moduleRoot=true))
    {
      InterfaceParser("test", <interface root="true"/>)
    }
  }
  
  def testInheritingInterface() {
    expectResult(apt.Interface(
      name="test",
      inherits=Some("supertest"),
      description=None,
      methods=Nil,
      signals=Nil, 
      properties=Nil, 
      moduleRoot=true))
    {
      InterfaceParser("test", <interface root="true" inherits="supertest"/>)
    }
  }
  
  def testExplicitlyNonRootInterface() {
    expectResult(apt.Interface(
      name="test",
      inherits=None, 
      description=None,
      methods=Nil, 
      signals=Nil, 
      properties=Nil, 
      moduleRoot=false))
    {
      InterfaceParser("test", <interface root="false"/>)
    }
  }
  
  def testDescribedInterface() {
    expectResult(apt.Interface(
      name="test",
      inherits=None,
      description=Some("My test interface"),
      methods=Nil,
      signals=Nil,
      properties=Nil,
      moduleRoot=false)) {
      InterfaceParser("test", 
        <interface>
          <description>My test interface</description>
        </interface>)
    }
  }

  def testSimpleProperty() {
    expectResult(apt.Property("prop", None, StringType, false, false) :: Nil) {
      InterfaceParser("test",
        <interface>
          <property name="prop" type="String" />
        </interface>
      ).properties
    }
  }
  
  def testReadOnlyProperty() {
    expectResult(apt.Property("prop", None, StringType, true, false) :: Nil) {
      InterfaceParser("test",
        <interface>
          <property name="prop" type="String" readonly="true" />
        </interface>
      ).properties
    }
  }
  
  def testInheritedProperty() {
    expectResult(apt.Property("prop", None, StringType, false, true) :: Nil) {
      InterfaceParser("test",
        <interface>
          <property name="prop" type="String" inherit="true" />
        </interface>
      ).properties
    }
  }
  
  def testDescribedProperty() {
    expectResult(apt.Property("prop", Some("TEST!"), StringType, false, false) :: Nil) {
      InterfaceParser("test",
        <interface>
          <property name="prop" type="String">
            <description>TEST!</description>
          </property>
        </interface>
      ).properties
    }
  }
  
  def testUnnamedProperty() {
    intercept[ParseErrorException] {
      InterfaceParser("test",
        <interface>
          <property type="String" />
        </interface>
      )
    }
  }

  def testUntypedProperty() {
    intercept[ParseErrorException] {
      InterfaceParser("test",
        <interface>
          <property name="prop"/>
        </interface>
      )
    }
  }

  def testSimpleSignal() {
    expectResult(apt.Signal("sig", None, Nil) :: Nil) {
      InterfaceParser("test",
        <interface>
          <signal name="sig" />
        </interface>
      ).signals
    }
  }

  def testParameterizedSignal() {
    expectResult(apt.Signal("sig", None, apt.Parameter("param", None, StringType, None, false) :: Nil) :: Nil) {
      InterfaceParser("test",
        <interface>
          <signal name="sig">
            <param name="param" type="String" />
          </signal>
        </interface>
      ).signals
    }
  }

  def testDescribedParameterizedSignal() {
    expectResult(apt.Signal("sig", Some("Some signal"), apt.Parameter("param", Some("Some parameter"), StringType, None, false) :: Nil) :: Nil) {
      InterfaceParser("test",
        <interface>
          <signal name="sig">
            <description>Some signal</description>
            <param name="param" type="String">
              <description>Some parameter</description>
            </param>
          </signal>
        </interface>
      ).signals
    }
  }

  def testSimpleMethod() {
    expectResult(apt.Method("meth", None, Nil, None) :: Nil) {
      InterfaceParser("test",
        <interface>
          <method name="meth" />
        </interface>
      ).methods
    }
  }
  
  def testReturnTypeMethod() {
    expectResult(apt.Method("meth", None, Nil, Some(StringType)) :: Nil) {
      InterfaceParser("test",
        <interface>
          <method name="meth" return="String" />
        </interface>
      ).methods
    }
  }
  
  def testParamMethod() {
    expectResult(apt.Method("meth", None, apt.Parameter("param1", None, IntegerType, None, false) :: apt.Parameter("param2", None, ObjectType, None, false) :: Nil, Some(StringType)) :: Nil) {
      InterfaceParser("test",
        <interface>
          <method name="meth" return="String">
            <param name="param1" type="Integer" />
            <param name="param2" type="Object" />
          </method>
        </interface>
      ).methods
    }
  }
  
  def testDefaultParamMethod() {
    expectResult(apt.Method("meth", None, apt.Parameter("param1", None, IntegerType, None, false) :: apt.Parameter("param2", None, BooleanType, Some(BooleanValue(true)), true) :: Nil, Some(StringType)) :: Nil) {
      InterfaceParser("test",
        <interface>
          <method name="meth" return="String">
            <param name="param1" type="Integer" />
            <param name="param2" type="Boolean" default="true" />
          </method>
        </interface>
      ).methods
    }
  }
  
  def testOptionalParamMethod() {
    expectResult(apt.Method("meth", None, apt.Parameter("param1", None, IntegerType, None, false) :: apt.Parameter("param2", None, ObjectType, None, true) :: Nil, Some(StringType)) :: Nil) {
      InterfaceParser("test",
        <interface>
          <method name="meth" return="String">
            <param name="param1" type="Integer" />
            <param name="param2" type="Object" optional="true" />
          </method>
        </interface>
      ).methods
    }
  }

  def testDescribedMethod() {
    expectResult(apt.Method("meth", Some("Method"), apt.Parameter("param1", Some("Parameter 1"), IntegerType, None, false) :: apt.Parameter("param2", Some("Parameter 2"), ObjectType, None, false) :: Nil, Some(StringType)) :: Nil) {
      InterfaceParser("test",
        <interface>
          <method name="meth" return="String">
            <description>Method</description>
            <param name="param1" type="Integer">
              <description>Parameter 1</description>
            </param>
            <param name="param2" type="Object">
              <description>Parameter 2</description>
            </param>
          </method>
        </interface>
      ).methods
    }
  }
}

// vim: set ts=4 sw=4 et:
