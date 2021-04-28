package com.ea.bridgedocgen.jsvalue

sealed abstract class JavaScriptValue {
  def webidlString : String
}

case class StringValue(value : String) extends JavaScriptValue {
  override def webidlString = "\"" + value + "\""
}

case class FloatValue(value : Double) extends JavaScriptValue {
  override def webidlString = value.toString
}

case class IntegerValue(value : Long) extends JavaScriptValue {
  override def webidlString = value.toString
}

case class BooleanValue(value : Boolean) extends JavaScriptValue {
  override def webidlString = value.toString
}

case object NullValue extends JavaScriptValue {
  override def webidlString = "null"
}
