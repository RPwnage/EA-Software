///////////////////////////////////////////////////////////////////////////////
// StoreJsHelper.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef STOREJSHELPER_H
#define STOREJSHELPER_H

#include "MyGameJsHelper.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Client
    {
        class ORIGIN_PLUGIN_API StoreJsHelper : public MyGameJsHelper
        {
            Q_OBJECT
        public:
            StoreJsHelper(QObject *parent = NULL);

        signals:
            void printRequested();

        public slots:
            // Everything in this function is commented out. However, removing it
            // would cause an error in the calling JS, so it needs to stay.
            void goOnline();
            void restartOrigin();
            void launchExternalBrowser(QString urlstring);
            void launchInicisSsoCheckout(QString urlparams);
            void continueShopping(const QString& url = QString());
            void print();
        };
    }
}

#endif