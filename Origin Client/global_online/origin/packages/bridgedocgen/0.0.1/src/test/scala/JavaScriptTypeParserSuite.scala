package test.scala

import com.ea.bridgedocgen.jstype._
import com.ea.bridgedocgen.JavaScriptTypeParser
import com.ea.bridgedocgen.ParseErrorException
import org.scalatest.Suite

class JavaScriptTypeParserSuite extends Suite {
  def expectParse(expected : Any)(actual : Any)  {
    actual match {
      case JavaScriptTypeParser.Success(parsedType, _) =>
        expectResult(expected)(parsedType)
      case err => fail(err.toString)
    }
  }

  def testIntegerType() {
    expectParse(IntegerType) {
      JavaScriptTypeParser("Integer")
    }
  }
  
  def testFloatType() {
    expectParse(FloatType) {
      JavaScriptTypeParser("Float")
    }
  }
  
  def testStringType() {
    expectParse(StringType) {
      JavaScriptTypeParser("String")
    }
  }
  
  def testObjectType() {
    expectParse(ObjectType) {
      JavaScriptTypeParser("Object")
    }
  }
  
  def testBooleanType() {
    expectParse(BooleanType) {
      JavaScriptTypeParser("Boolean")
    }
  }
  
  def testDateType() {
    expectParse(DateType) {
      JavaScriptTypeParser("Date")
    }
  }
  
  def testCustomType() {
    expectParse(CustomType("Entitlement")) {
      JavaScriptTypeParser("Entitlement")
    }
  }

  def testStringArray() {
    expectParse(ArrayType(StringType)) {
      JavaScriptTypeParser("String[]")
    }
  }

  def testNullableString() {
    expectParse(NullableType(StringType)) {
      JavaScriptTypeParser("String?")
    }
  }
  
  def testNullableObjectDoubleArray() {
    expectParse(NullableType(ArrayType(ArrayType(ObjectType)))) {
      JavaScriptTypeParser("Object[][]?")
    }
  }

  def testAnyType() {
    expectParse(AnyType) {
      JavaScriptTypeParser("Any")
    }
  }

  def testSimpleUnion() {
    expectParse(UnionType(BooleanType :: StringType :: Nil)) {
      JavaScriptTypeParser("(Boolean or String)")
    }
  }
  
  def testSpacedSimpleUnion() {
    expectParse(UnionType(BooleanType :: StringType :: Nil)) {
      JavaScriptTypeParser("(    Boolean    or   String   )")
    }
  }
  
  def testNullableSimpleUnion() {
    expectParse(NullableType(UnionType(BooleanType :: StringType :: Nil))) {
      JavaScriptTypeParser("(Boolean or String)?")
    }
  }
  
  def testNullableMemberSimpleUnion() {
    expectParse(UnionType(BooleanType :: NullableType(StringType) :: Nil)) {
      JavaScriptTypeParser("(Boolean or String?)")
    }
  }
  
  def testNestedUnion() {
    expectParse(UnionType(UnionType(List(DateType, IntegerType, ObjectType)) :: UnionType(List(ArrayType(FloatType), NullableType(FloatType))) :: Nil)) {
      JavaScriptTypeParser("((Date or Integer or Object) or (Float[] or Float?))")
    }
  }
}

// vim: set ts=4 sw=4 et:
