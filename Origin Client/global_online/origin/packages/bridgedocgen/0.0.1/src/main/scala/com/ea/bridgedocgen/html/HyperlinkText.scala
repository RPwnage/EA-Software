package com.ea.bridgedocgen.html
import scala.xml

object HyperlinkText {
  def apply(linkWords : Map[String,String])(inputText : String) : xml.NodeSeq = {
    val outputWords = inputText.split("""\s+""") map { inputWord =>
      linkWords get inputWord match {
        case Some(url) =>
          // Link the word
          <a href={url}>{inputWord}</a>
        case None =>
          // Pass it through
          xml.Text(inputWord)
      }
    }

    outputWords.foldLeft(xml.NodeSeq.Empty) { (list, current) =>
      if (list.isEmpty) {
        current :: Nil
      }
      else {
        list ++ xml.Text(" ") ++ current
      }
    }
  }
}
