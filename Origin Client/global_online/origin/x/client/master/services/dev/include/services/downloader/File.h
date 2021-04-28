//
// Copyright: Electronic Arts 2008
//
// Author: Craig Nielsen
//
// Hugely based on existing code in File and File
//
// Reason:
//	   We lost CFIle and it's variants in a move away from MFC for Core Service so I
//	   replaced these with wrappers around some open source replacements in case we
//     need to replace these due to licensing or usage issues later.
//
#pragma once

#include <QFile>
#include <QString>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Downloader
{

	class ORIGIN_PLUGIN_API File : public QFile
	{
		public:

			File(const QString& name) : QFile(name) {}
			File(QObject* parent) : QFile(parent) {}
			File(const QString& name, QObject* parent) : QFile(name, parent) {}
	};

} // Downloader
} // Origin