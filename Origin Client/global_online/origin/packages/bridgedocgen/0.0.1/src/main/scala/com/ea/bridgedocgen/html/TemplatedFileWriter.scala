package com.ea.bridgedocgen.html

import java.io.File
import java.io.FileWriter
import java.text.SimpleDateFormat
import java.util.Date
import scala.xml._

object TemplatedFileWriter {
  def apply(file : File, title : String, body : NodeSeq, linker : LinkBuilder, widlLink : Option[NodeSeq]) {
    file.createNewFile

    val staticRoot = linker.urlFor("static/")

    val xhtmlNodes = 
      <html xmlns="http://www.w3.org/1999/xhtml">
        <head>
          <title>{ title }</title>
          <link rel="stylesheet" type="text/css" href={ staticRoot + "style.css" } />
        </head>
        <body>
          { body }
          <footer>
            { widlLink.getOrElse(NodeSeq.Empty) }
            <span class="generated-date">Generated on { new SimpleDateFormat("yyyy-MM-dd HH:mm:ss z").format(new Date) }</span> 
          </footer>
        </body>
      </html>
    
    val writer = new FileWriter(file)
    writer.write("<!DOCTYPE html>\n")
    writer.write(Xhtml.toXhtml(xhtmlNodes))
    writer.close
  }
}
