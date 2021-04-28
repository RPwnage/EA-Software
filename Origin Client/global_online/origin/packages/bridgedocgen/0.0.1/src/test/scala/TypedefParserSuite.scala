package test.scala

import com.ea.bridgedocgen._
import jstype._
import org.scalatest.Suite
import scala.xml

class TypedefParserSuite extends Suite {
  def testSimpleTypedef() {
    expectResult(apt.Typedef("test", None, StringType)) {
      TypedefParser("test", <typedef type="String"/>)
    }
  }
  
  def testDocumentedTypedef() {
    expectResult(apt.Typedef("test", Some("Test description"), NullableType(DateType))) {
      TypedefParser("test", 
        <typedef type="Date?">
          <description>Test description</description>
        </typedef>)
    }
  }
}

// vim: set ts=4 sw=4 et:

// vim: set ts=4 sw=4 et:
