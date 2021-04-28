///////////////////////////////////////////////////////////////////////////////
// StringUtils.cpp
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "engine/utilities/StringUtils.h"

#include <QStringList>

namespace Origin
{
    namespace StringUtils
    {

        QString formatLocaleListAsQString(const QStringList& localeList)
        {
            return localeList.join(",").remove('_');
        }

    } // namespace StringUtils

} // namespace Origin
