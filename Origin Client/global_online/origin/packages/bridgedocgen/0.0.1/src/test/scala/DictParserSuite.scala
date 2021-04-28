package test.scala

import com.ea.bridgedocgen._
import jstype._
import org.scalatest.Suite
import scala.xml

class DictParserSuite extends Suite {
  def testEmptyDict() {
    expectResult(apt.Dict(
      name="test", 
      inherits=None,
      description=None, 
      members=Nil)) 
    {
      DictParser("test", <dict/>)
    }
  }

  def testDescribedDict() {
    expectResult(apt.Dict(
      name="test", 
      inherits=None,
      description=Some("Some description"),
      members=Nil)) {
      DictParser("test",
        <dict>
          <description>Some description</description>
        </dict>
      )
    }
  }
  
  def testInheritingDict() {
    expectResult(apt.Dict(
      name="test", 
      inherits=Some("superdict"),
      description=None, 
      members=Nil)) 
    {
      DictParser("test", <dict inherits="superdict"/>)
    }
  }
  
  def testSimpleMember() {
    expectResult(apt.DictMember("member", None, NullableType(StringType)) :: Nil) {
      DictParser("test",
        <dict>>
          <member name="member" type="String?" />
        </dict>
      ).members
    }
  }
  
  def testDescribedMember() {
    expectResult(apt.DictMember("member", Some("Some member"), DateType) :: Nil) {
      DictParser("test",
        <dict>>
          <member name="member" type="Date">
            <description>Some member</description>
          </member>
        </dict>
      ).members
    }
  }
  
  def testMultiMember() {
    expectResult(apt.DictMember("member1", None, DateType) :: apt.DictMember("member2", None, AnyType) :: Nil) {
      DictParser("test",
        <dict>>
          <member name="member1" type="Date"/>
          <member name="member2" type="Any"/>
        </dict>
      ).members
    }
  }
  
}

// vim: set ts=4 sw=4 et:
