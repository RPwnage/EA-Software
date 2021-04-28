package com.ea.bridgedocgen

import java.io.File
import scala.xml.{XML,Elem}

import javax.xml.XMLConstants
import javax.xml.transform.Source
import javax.xml.transform.stream.StreamSource
import javax.xml.validation.{Validator, SchemaFactory}

object ModuleDirectoryParser {
  private case class DefinitionFile(baseName : String, file : File);
  
  private def findDefinitionFiles(root : File, typeName : String) : Seq[DefinitionFile] = {
    val candidateFiles = for(candidate <- root.listFiles if candidate.isFile)
      yield candidate

    // See if the name matches
    val FilenameMatch = ("""(.*)\.""" + typeName + "$").r

    return candidateFiles flatMap { candidateFile =>
      candidateFile.getName match {
        case FilenameMatch(baseName) =>
          DefinitionFile(baseName, candidateFile) :: Nil
        case _ => 
          Nil
      }
    }
  }

  private def loadFileWithDiagnostics(file : File, schemaName : String) : Elem = {

    try {
      // Validate the file's schema first
      val schemaFactory = SchemaFactory.newInstance(XMLConstants.W3C_XML_SCHEMA_NS_URI)
      val xsdStream = getClass.getClassLoader.getResourceAsStream(s"schemas/${schemaName}.xsd")
      val schema = schemaFactory.newSchema(new StreamSource(xsdStream))
      val validator = schema.newValidator

      validator.validate(new StreamSource(file))
      
      // Now actually load the file
      XML.loadFile(file)
    } catch {
      case parseException : org.xml.sax.SAXParseException =>
        // Prepend the error location
        val filename = file.getPath()
        val line = parseException.getLineNumber()
        val message = s"$filename:$line: " + parseException.getMessage()

        throw new XMLException(message)
    }
  }

  def apply(root : File) = {
    // Find our interface files 
    val interfaceFiles = findDefinitionFiles(root, "interface")
    val interfaces = interfaceFiles map { interfaceFile =>
        InterfaceParser(interfaceFile.baseName, loadFileWithDiagnostics(interfaceFile.file, "interface"))
    }
    
    // Find our enums
    val enumFiles = findDefinitionFiles(root, "enum")
    val enums = enumFiles map { enumFile =>
        EnumParser(enumFile.baseName, loadFileWithDiagnostics(enumFile.file, "enum"))
    }
    
    // Find our dicts
    val dictFiles = findDefinitionFiles(root, "dict")
    val dicts = dictFiles map { dictFile =>
        DictParser(dictFile.baseName, loadFileWithDiagnostics(dictFile.file, "dict"))
    }
    
    // Find our typedefs
    val typedefFiles = findDefinitionFiles(root, "typedef")
    val typedefs = typedefFiles map { typedefFile =>
        TypedefParser(typedefFile.baseName, loadFileWithDiagnostics(typedefFile.file, "typedef"))
    }

    (interfaces, enums, dicts, typedefs)
  }
}
