/////////////////////////////////////////////////////////////////////////////
// PromoJsHelper.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef PromoJsHelper_H
#define PromoJsHelper_H

#include <QObject>
#include <QString>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
	namespace Client
	{
		class ORIGIN_PLUGIN_API PromoJsHelper : public QObject
		{
			Q_OBJECT
		public:
			PromoJsHelper( QObject *parent = NULL );

		public slots:
			void launchBrowserWithSingleSignOn( QString url );
			void viewProductOnStoreTab( const QString& );
			void viewUrlOnStoreTab( const QString& );
            void viewGamesDetailPage(const QString& ids);
            void close();

        signals:
            void closeWindow();
		};
	}
}

#endif