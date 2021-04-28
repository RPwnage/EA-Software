package com.ea.bridgedocgen

import scopt.mutable.OptionParser
import java.io.File
import html.HTMLWriter

object Main extends App {
  var inputPath : String = null
  var outputPath : Option[String] = None
  var dryRun : Boolean = false

  val parser = new OptionParser("bridgedocgen") {
    arg("<inputdir>", "Root of the JavaScript interface documentation", {v : String => inputPath = v })
    opt("o", "output", "<dir>", "Output HTML directory", {v : String => outputPath = Some(v) })
    opt("dry-run",  "Parses the documentation without producing output",  { dryRun = true })
  }

  if (!parser.parse(args)) {
    sys.exit(1)
  }

  val inputDir = new File(inputPath)

  try {
    if (!inputDir.exists) {
      throw new ParseErrorException("Directory does not exist: " + inputPath)
    }

    val definitions = InputDirectoryParser(inputDir)

    if (!dryRun) {
      // Find our output dir
      val outputDir = outputPath match {
        case Some(path) => new File(path)
        // Default to a subdir of the input dir
        case None => new File(inputDir, "html")
      }

      // Create our output directory
      outputDir.mkdir

      // Write!
      HTMLWriter(outputDir, definitions)
    }
  } catch {
    case e : XMLException =>
      System.err.println("XML error: " + e.getMessage())
      sys.exit(2)
    case e : ParseErrorException =>
      System.err.println("Parse error: " + e.getMessage())
      sys.exit(3)
    case e : SemanticException =>
      System.err.println("Semantic error: " + e.getMessage())
      sys.exit(4)
  }
}
