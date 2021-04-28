#ifndef _CLOUDSAVES_CLOUDDATEFORMAT_H
#define _CLOUDSAVES_CLOUDDATEFORMAT_H

#include <QDateTime>
#include <QString>
#include <QLocale>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
	namespace CloudDateFormat
	{
		ORIGIN_PLUGIN_API QString format(const QTime& time, const QLocale &locale = QLocale());
		ORIGIN_PLUGIN_API QString format(const QDate& date, const QLocale &locale = QLocale());
		ORIGIN_PLUGIN_API QString format(const QDateTime& datetime, const QLocale &locale = QLocale());
	}
}
}
}
#endif