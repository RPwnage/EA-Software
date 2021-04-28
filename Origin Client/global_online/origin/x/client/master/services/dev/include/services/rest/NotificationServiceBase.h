///////////////////////////////////////////////////////////////////////////////
// NotificationServiceBase.h
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _NOTIFICATIONSERVICEBASE_H_INCLUDED_
#define _NOTIFICATIONSERVICEBASE_H_INCLUDED_

#include "OriginServiceClient.h"
#include <QMap>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        class ORIGIN_PLUGIN_API NotificationServiceBase : public QObject
        {
		
		Q_OBJECT
		
		public:

			/// \brief DTOR
			virtual ~NotificationServiceBase() = 0;
			
            /// \fn token
            /// \brief allows the retrieval of any value from the map of values.
            /// \brief this way of retrieval allows the retrieval of values that we might not even
            /// \brief consider at design time.
            QString token(const QString& token_name) const;			
			
		signals:
		
		// signal emited when all is done
		void finished();
		
		protected slots:
		
		private slots:
		
		protected:
            /// \brief CTOR
            explicit NotificationServiceBase(QObject *parent = 0);
			
        protected:		
            /// \brief contains the required data
            QMap<QString, QString> mUserInfo;
			
        private:

		};
	}
}
#endif

