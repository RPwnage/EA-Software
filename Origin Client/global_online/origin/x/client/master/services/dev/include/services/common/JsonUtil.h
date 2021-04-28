//    Copyright © 2011-2013, Electronic Arts
//    All rights reserved.

#ifndef JSONUTIL_H
#define JSONUTIL_H

#include <vector>

#include <qbytearray.h>
#include <qstring.h>
#include <qvariant.h>

#include "libjson.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{

namespace Services
{

namespace JsonUtil
{

	typedef QMap<QString, QVariant> JsonDom;
	typedef std::vector<libjson::json_pair *> JsonKV;

	/*

	Sample usage, given JSON that looks like:

		{
		  "userProfileEntry": [{
			"pidUri": "/pids/19056",
			"entryId": "216350731",
			"entryCategory": "MOBILE_INFO",
			"entryElement": [{
			  "profileType": "MOBILE_CARRIER",
			  "value": "SPRINT"
			},
			{
			  "profileType": "MOBILE_MODEL",
			  "value": "T130"
			},
			{
			  "profileType": "MOBILE_MAKE",
			  "value": "SAMSUNG"
			}]
		  }]
		}

	The code to interact with it could look like:

		JsonUtil::JsonWrapper dom;

		dom.parse(lines); // lines is a QByteArray containing the JSON

		QString pidUri = dom.search("pidUri").toString(); // returns a QVariant, will be an empty string if stuff changes

		- or, if you need to access the plumbing directly -

		QString pidUri = dom.dom["userProfileEntry"].toMap()["0"].toMap()["pidUri"].toString();		
	*/


	class ORIGIN_PLUGIN_API JsonWrapper
	{
	public:

		bool parse(const QByteArray& ba);
		QVariant search(const QString& key);

		void clear();

		JsonDom dom;
		libjson::json parser;

	private:

		QVariant search(JsonDom &dom, const QString& key);

		QVariant parseItem(libjson::json &parser, libjson::json_value* val);
		QMap<QString, QVariant> parseArray(libjson::json &parser, libjson::json_value* val);
		QMap<QString, QVariant> parseObject(libjson::json &parser, libjson::json_value* val);
	};
}

}

}

#endif
