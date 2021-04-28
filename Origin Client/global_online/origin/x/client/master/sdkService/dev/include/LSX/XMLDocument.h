///////////////////////////////////////////////////////////////////////////////
//
//  Copyright © 2008 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
//
//  File: XMLDocument.h 
//	generic XML wrapper class specification
//
//	Author: Sergey Zavadski

#ifndef _XML_DOCUMENT_H_
#define _XML_DOCUMENT_H_

class QDomDocument;
class QDomNode;
class QDomNodeList;
class QString;

#include <QVector>

namespace Origin
{
    namespace SDK
    {
        namespace Lsx
        {
            // Loads the XML content from a file referenced by 'filePath' into the given QDomDocument.
            // Returns true if the save operation succeeded.
            bool LoadXmlFromFile(QDomDocument& documentToLoad, QString const& filePath);

            // Saves the QDomDocument into the file referenced by 'filePath'.
            // Returns true if the save operation succeeded.
            bool SaveXmlToFile(QDomDocument& documentToLoad, QString const& filePath);

            // Selects the given content from the node specified by 'sourceNode' according to the criteria
            // specified in 'xmlQuery' and returns the selection as XML in a string.
            typedef QVector<QDomNode> SelectedDomNodes;
        }
    }
}

#endif // _XML_DOCUMENT_H_
