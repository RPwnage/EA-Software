package test.scala

import com.ea.bridgedocgen.jsvalue
import com.ea.bridgedocgen.jstype
import com.ea.bridgedocgen.JavaScriptValueParser
import com.ea.bridgedocgen.ParseErrorException
import org.scalatest.Suite

class JavaScriptValueParserSuite extends Suite {
  def testStringValue {
    expectResult(jsvalue.StringValue("Hello, world!")) {
      JavaScriptValueParser(jstype.StringType)("Hello, world!")
    }
  }
  
  def testNullableStringValue {
    expectResult(jsvalue.StringValue("Hello, world!")) {
      JavaScriptValueParser(jstype.NullableType(jstype.StringType))("Hello, world!")
    }
  }
  
  def testNulledStringValue {
    expectResult(jsvalue.NullValue) {
      JavaScriptValueParser(jstype.NullableType(jstype.StringType))("null")
    }
  }

  def testEnumValue {
    expectResult(jsvalue.StringValue("VALUE1")) {
      JavaScriptValueParser(jstype.CustomType("MyEnum"))("VALUE1")
    }
  }
  
  def testIntegerValue {
    expectResult(jsvalue.IntegerValue(450)) {
      JavaScriptValueParser(jstype.IntegerType)("450")
    }
  }

  def testBadIntegerValue {
    intercept[ParseErrorException] {
      JavaScriptValueParser(jstype.IntegerType)("Cats!")
    }
  }
  
  def testFloatValue {
    expectResult(jsvalue.FloatValue(-15.0)) {
      JavaScriptValueParser(jstype.FloatType)("-15.0")
    }
  }

  def testBadFloatValue {
    intercept[ParseErrorException] {
      JavaScriptValueParser(jstype.FloatType)("Dogs!")
    }
  }

  def testTrueBooleanValue {
    expectResult(jsvalue.BooleanValue(true)) {
      JavaScriptValueParser(jstype.BooleanType)("true")
    }
  }

  def testFalseBooleanValue {
    expectResult(jsvalue.BooleanValue(false)) {
      JavaScriptValueParser(jstype.BooleanType)("false")
    }
  }
  
  def testBadBooleanValue {
    intercept[ParseErrorException] {
      JavaScriptValueParser(jstype.BooleanType)("maybe")
    }
  }
}

// vim: set ts=4 sw=4 et:
