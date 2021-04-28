/////////////////////////////////////////////////////////////////////////////
// MOTDJsHelper.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#ifndef MOTDJsHelper_H
#define MOTDJsHelper_H

#include <QObject>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
	namespace Client
	{
		class ORIGIN_PLUGIN_API MOTDJsHelper : public QObject
		{
			Q_OBJECT

		public:
			MOTDJsHelper(QObject *parent = NULL);

			public slots:
				void launchSystemBrowser( QString url );
		};
	}
}

#endif