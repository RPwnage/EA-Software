///////////////////////////////////////////////////////////////////////////////
// NotificationServiceBase.cpp
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "services/rest/NotificationServiceBase.h"

namespace Origin
{
    namespace Services
    {

        NotificationServiceBase::NotificationServiceBase( QObject *parent /*= 0*/ )
        {

        }

        QString NotificationServiceBase::token( const QString& key ) const
        {
            QString value = "";

            if (mUserInfo.contains(key.toUpper()))
            {
                value = mUserInfo[key.toUpper()];
            }
            return value;

        }

        NotificationServiceBase::~NotificationServiceBase()
        {

        }

    }
}