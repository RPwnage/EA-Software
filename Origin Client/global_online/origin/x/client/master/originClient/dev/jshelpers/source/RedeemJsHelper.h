///////////////////////////////////////////////////////////////////////////////
// RedeemJsHelper.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef REDEEMJSHELPER_H
#define REDEEMJSHELPER_H

#include "MyGameJsHelper.h"
#include <QString>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Client
    {
        class ORIGIN_PLUGIN_API RedeemJsHelper : public MyGameJsHelper
        {
            Q_OBJECT
        public:
            enum RedeemType{ITE, RTP, Origin};

            RedeemJsHelper(RedeemType redeemType, QObject *parent = NULL);

        signals:
            void windowCloseRequested();	

        public slots:
            void codeRedeemed();
            void faqRequested();
            void redeemError(int errorCode);
            void next();
            void close();

        private:
            RedeemType mRedeemType;
        };
    }
}

#endif