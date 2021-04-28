#include "stdafx.h"
#include "XmlDocument.h"
#include "RapidXmlParser.h"


INodeDocument * CreateRapidXmlDocument()
{
	return new RapidXmlParser();
}

