///////////////////////////////////////////////////////////////////////////////
// ChooseAvatarJsHelper.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef CHOOSEAVATARJSHELPER_H
#define CHOOSEAVATARJSHELPER_H

#include <QObject>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Client
    {
        class ORIGIN_PLUGIN_API ChooseAvatarJsHelper : public QObject
        {
            Q_OBJECT

        public:
            ChooseAvatarJsHelper(QObject *parent = NULL);

        public slots:
            void cancel();
            void updateAvatar(QString avatarID);	
        signals:
            void cancelled();
            void updateAvatarRequested(QString);
        };
    }
}

#endif