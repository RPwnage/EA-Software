package com.ea.bridgedocgen
import java.io.File
import scala.io.Source

object InputDirectoryParser {
  def apply(root : File) : apt.Definitions = {
    var featureModules : List[apt.FeatureModule] = List()
    var commonModule : Option[apt.CommonModule] = None
    
    for(moduleDir <- root.listFiles; if moduleDir.isDirectory; if moduleDir.getName != "html") {
      val (interfaces, enums, dicts, typedefs) = ModuleDirectoryParser(moduleDir)

      if (moduleDir.getName == "common") {
        commonModule = Some(apt.CommonModule(interfaces, enums, dicts, typedefs))
      }
      else {
        val readmeFile = new File(moduleDir, "README")

        // Try to load a README file
        val description : Option[String] = try {
          Some(scala.io.Source.fromFile(readmeFile).mkString)
        } catch {
          case e : java.io.FileNotFoundException => 
            // No README file
            interfaces match {
              case Seq(singleInterface) =>
                // Inherit the description of our only interface
                singleInterface.description
              case _ =>
                // Can't find a description
                None
            }
        }

        val featureModule = apt.FeatureModule(moduleDir.getName, description, interfaces, enums, dicts, typedefs)
        featureModules = featureModule :: featureModules
      }
    }

    apt.Definitions(commonModule, featureModules)
  }
}
