package com.ea.bridgedocgen

import java.io.File

abstract class DocumentationWriter {
  def apply(outputDir : File, definitions : apt.Definitions)
}
