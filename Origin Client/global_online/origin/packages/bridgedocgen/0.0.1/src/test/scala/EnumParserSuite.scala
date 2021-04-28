package test.scala

import com.ea.bridgedocgen._
import org.scalatest.Suite
import scala.xml

class EnumParserSuite extends Suite {
  def testEmptyEnum() {
    expectResult(apt.Enum("test", None, Nil)) {
      EnumParser("test", <enum/>)
    }
  }
  
  def testDescribedEnum() {
    expectResult(apt.Enum("test", Some("Test enum"), Nil)) {
      EnumParser("test", 
        <enum>
          <description>Test enum</description>
        </enum>)
    }
  }
  
  def testSingleEnumValue() {
    expectResult(apt.Enum("test", None, apt.EnumValue("READY_TO_PLAY", None) :: Nil)) {
      EnumParser("test", 
        <enum>
          <value name="READY_TO_PLAY"/>
        </enum>)
    }
  }
  
  def testDescribedEnumValue() {
    expectResult(apt.Enum("test", None, apt.EnumValue("READY_TO_PLAY", Some("Content is ready to play")) :: Nil)) {
      EnumParser("test", 
        <enum>
          <value name="READY_TO_PLAY">
            <description>Content is ready to play</description>
          </value>
        </enum>)
    }
  }

  def testMultiEnumValue() {
    expectResult(apt.Enum("test", None, 
      apt.EnumValue("READY_TO_PLAY", None) :: apt.EnumValue("DOWNLOADING", None) :: Nil)) {
      EnumParser("test", 
        <enum>
          <value name="READY_TO_PLAY"/>
          <value name="DOWNLOADING"/>
        </enum>)
    }
  }
}

// vim: set ts=4 sw=4 et:
