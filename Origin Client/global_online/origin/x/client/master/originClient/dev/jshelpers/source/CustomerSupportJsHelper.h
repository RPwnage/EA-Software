///////////////////////////////////////////////////////////////////////////////
// CustomerSupportJsHelper.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef CUSTOMERSUPPORTJSHELPER_H
#define CUSTOMERSUPPORTJSHELPER_H

#include <QObject>

#include "services/plugin/PluginAPI.h"

class QWebView;

namespace Origin
{
    namespace Client
    {
        class ORIGIN_PLUGIN_API CustomerSupportJsHelper : public QObject
        {
            Q_OBJECT
        public:
            enum Scope { DEFAULT, IGO };

            CustomerSupportJsHelper(Scope location, QObject *parent = NULL);

            public slots:
                bool canNavigateNext();
                bool canNavigatePrevious();
                void openLink(QString url);

        private:
            Scope mScope;
            QWebView* mParentWebView;
        };
    }
}

#endif