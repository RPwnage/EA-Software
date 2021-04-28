///////////////////////////////////////////////////////////////////////////////
// OriginTranslator.cpp
// 
// Created by Kevin Moore
// Copyright (c) 2013 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "OriginTranslator.h"
#include <QVariant>

OriginTranslator::OriginTranslator()
{

}
OriginTranslator::~OriginTranslator()
{

}

QString OriginTranslator::translate(const char *context, const char *sourceText, const char *disambiguation, int n) const
{
    QString sTranslated = EATranslator::translate(context, sourceText, disambiguation);

    if ( !sTranslated.isEmpty() )
    {
        QMap<QString, QString>::const_iterator it = mTokenMap.begin();

        while ( it != mTokenMap.end() )
        {
            sTranslated.replace(it.key(), it.value());
            it++;
        }
    }

    return sTranslated;
}

void OriginTranslator::addToken(const QString& sToken, const QString& sValue)
{
    mTokenMap[sToken] = sValue;
}

void OriginTranslator::removeToken(const QString& sToken)
{
    mTokenMap.remove(sToken);
}
