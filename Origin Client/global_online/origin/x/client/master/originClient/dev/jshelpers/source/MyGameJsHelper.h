///////////////////////////////////////////////////////////////////////////////
// MyGameJsHelper.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef _MYGAMEJSHELPER_H_INCLUDED 
#define _MYGAMEJSHELPER_H_INCLUDED 

#include <QObject>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Client
    {
        class ORIGIN_PLUGIN_API MyGameJsHelper  : public QObject
        {
            Q_OBJECT

        public:
            ///
            /// CTOR/DTOR
            ///
            MyGameJsHelper(QObject *parent = NULL);
            virtual ~MyGameJsHelper(void);

        public slots:
            ///
            /// refresh games window
            ///
            virtual void refreshEntitlements();
            ///
            /// start game download
            /// \param id The content id.
            ///
            virtual void startDownload(const QString& id);
            ///
            /// shows the game details page for the given entitlement
            /// \param id The content id.
            ///
            virtual void showGameDetails(const QString& id);
            /// 
            /// show games window
            ///
            virtual void showGames();
            ///
            /// moves to store tab in the client
            ///
            virtual void loadStoreHome();
            ///
            /// \brief Provides a basic state description of an entitlement given its id
            /// \param id The content or offer id of the entitlement
            ///
            virtual QString getState(const QString& id);

        private:
            ///
            /// disabling not needed functions
            ///
            MyGameJsHelper( const MyGameJsHelper&);
            MyGameJsHelper& operator=(const MyGameJsHelper&);
        };
    }
}

#endif


