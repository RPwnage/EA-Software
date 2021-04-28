package com.ea.bridgedocgen.html

import java.io.File
import java.io.FileOutputStream
import java.io.OutputStreamWriter

import scala.xml.NodeSeq
import scala.io.Source

import com.ea.bridgedocgen.apt
import com.ea.bridgedocgen.DocumentationWriter
import com.ea.bridgedocgen.webidl

object HTMLWriter extends DocumentationWriter {
  private def copyFileFromResource(resourceName : String, outputFile : File) {
    // Open the input stream
    val inStream = getClass.getClassLoader.getResourceAsStream(resourceName)
    val buffer = new Array[Byte](inStream.available)
    inStream.read(buffer)
 
    // Create an output stream
    val outStream = new FileOutputStream(outputFile)
    // Write it our
    outStream.write(buffer)
    outStream.close
  }

  private def writeTemplate(outputFile : File, doc : HTMLDocumentation, widlLink : Option[NodeSeq])(implicit linker : LinkBuilder) {
    TemplatedFileWriter(outputFile, doc.title, <h1>{doc.title}</h1> ++ doc.content, linker, widlLink) 
  }

  private def writeUtf8File(outputFile : File, content : String) {
    val outStream = new OutputStreamWriter(new FileOutputStream(outputFile), "UTF8")
    
    outStream.write(content)
    outStream.close
  }

  def writeDefinitionFiles[T <: apt.NamedDefinition](root : File, module : apt.DefinitionModule, definition : T, htmlBuilder : (apt.DefinitionModule, T) => HTMLDocumentation, webidlBuilder : (T) => String)(implicit linker : LinkBuilder) {
    val xhtmlFile = new File(root, definition.name + ".xhtml")
    writeTemplate(xhtmlFile, htmlBuilder(module, definition), Some(linker.linkToWidl(module, definition)))

    val webidlFile = new File(root, definition.name + ".widl")
    writeUtf8File(webidlFile, webidlBuilder(definition))
  }

  def apply(outputDir : File, definitions : apt.Definitions) {
    // Make our static/ directory
    val staticDir = new File(outputDir, "static")
    staticDir.mkdir()

    // Copy over our stylesheet
    copyFileFromResource("style.css", new File(staticDir, "style.css"))

    // Write our front page
    val linker = new LinkBuilder(definitions.modules, 0)
    val indexFile = new File(outputDir, "index.xhtml")
    writeTemplate(indexFile, FrontPageBuilder(definitions)(linker), None)(linker)

    for(module <- definitions.modules) {
      implicit val nestedLinker = linker.nestedLinker

      // Make the directory for the module
      val moduleRoot = new File(outputDir, module.name)
      moduleRoot.mkdir

      // Write out the top level module file
      val moduleFile = new File(moduleRoot, "index.xhtml")
      writeTemplate(moduleFile, ModuleDetailBuilder(module), Some(nestedLinker.linkToWidl(module)))

      // Write out the interfaces
      for(interface <- module.interfaces) {
        writeDefinitionFiles(moduleRoot, module, interface, InterfaceDetailBuilder.apply, webidl.InterfaceDefinitionBuilder.apply)
      }
      
      // Write out the enums
      for(enum <- module.enums) {
        writeDefinitionFiles(moduleRoot, module, enum, EnumDetailBuilder.apply, webidl.EnumDefinitionBuilder.apply)
      }
      
      // Write out the dicts
      for(dict <- module.dicts) {
        writeDefinitionFiles(moduleRoot, module, dict, DictDetailBuilder.apply, webidl.DictDefinitionBuilder.apply)
      }
      
      // Write out the typedefs
      for(typedef <- module.typedefs) {
        writeDefinitionFiles(moduleRoot, module, typedef, TypedefDetailBuilder.apply, webidl.TypedefDefinitionBuilder.apply)
      }

      // Write out our WebIDL
      val webidlFile = new File(moduleRoot, module.name + "Definitions.widl")
      writeUtf8File(webidlFile, webidl.ModuleDefinitionBuilder(module))
    }
  }
}
