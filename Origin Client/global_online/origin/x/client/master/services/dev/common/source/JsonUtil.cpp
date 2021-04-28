#include "services/common/JsonUtil.h"

#include <string>

namespace
{
	QString strip(std::string str)
	{
		QString temp = QString::fromUtf8(str.c_str());

		if (temp.startsWith('"'))
		{
			// Strip the leading, and trailing quotes.
			temp = temp.mid(1, temp.length()-2);
		}

		return temp;
	}
}

namespace Origin
{
	namespace Services
	{
		namespace JsonUtil
		{
			QMap<QString, QVariant> JsonWrapper::parseArray(libjson::json &parser, libjson::json_value* val)
			{
				QMap<QString, QVariant> list;

				int i = 0;
				for(std::vector<libjson::json_value*>::iterator it = val->array.begin(); it != val->array.end(); it++, i++)
				{
					list[QString::number(i)] = parseItem(parser, *it);
				}

				return list;
			}

			QMap<QString, QVariant> JsonWrapper::parseObject(libjson::json &parser, libjson::json_value* val)
			{
				QMap<QString, QVariant> object;

				for(std::vector<libjson::json_pair*>::iterator it = val->object.pairs.begin(); it != val->object.pairs.end(); it++)
				{
					QString key = strip((*it)->name.toString(parser.content));
					object[key] = parseItem(parser, &(*it)->value);
				}

				return object;
			}

			QVariant JsonWrapper::parseItem(libjson::json &parser, libjson::json_value* val)
			{
				if (val->isArray)
				{
					return QVariant(parseArray(parser, val));
				}
				else if (val->isObject)
				{
					return QVariant(parseObject(parser, val));
				}
				else
				{
					return QVariant(strip(val->value.toString(parser.content)));
				}
			}

			QVariant JsonWrapper::search(QMap<QString, QVariant> &dom, const QString& key)
			{
				QVariant result;

				for(QMap<QString, QVariant>::iterator it = dom.begin(); it != dom.end() && !result.isValid(); it++)
				{
					if (it.key() == key)
					{
						result = it.value();
					}
					else if (it.value().canConvert<QMap<QString, QVariant> >())
					{
                        QMap<QString, QVariant> map = it.value().toMap();
						result = search(map, key);
					}
				}

				return result;
			}

			QVariant JsonWrapper::search(const QString& key)
			{
				return search(dom, key);
				
			}

			bool JsonWrapper::parse(const QByteArray &ba)
			{
				bool parsed = parser.parse(ba.data());

				if (!parsed) return false;

				// walk top level children
				for(JsonKV::iterator it = parser.object->pairs.begin(); it != parser.object->pairs.end(); it++)
				{
					QVariant value = parseItem(parser, &(*it)->value);

					dom[strip((*it)->name.toString(parser.content))] = value;

					//ORIGIN_LOG_EVENT << strip((*it)->name.toString(parser.content)) << " | " << strip((*it)->value.toString(parser.content));
				}

				return true;
			}
		}
	}
}
