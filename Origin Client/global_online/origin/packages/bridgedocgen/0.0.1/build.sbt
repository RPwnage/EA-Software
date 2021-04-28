name := "bridgedocgen"

version := "0.0.1"

organization := "com.ea.bridgedocgen"

scalaVersion := "2.10.2"

libraryDependencies += "org.scalatest" %% "scalatest" % "1.9.1" % "test"

libraryDependencies += "com.github.scopt" %% "scopt" % "2.1.0"

scalacOptions ++= Seq("-deprecation", "-feature")
