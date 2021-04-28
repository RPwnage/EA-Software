///////////////////////////////////////////////////////////////////////////////
// OriginTranslator.h
// 
// Created by Kevin Moore
// Copyright (c) 2013 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef ORIGIN_TRANSLATOR_H
#define ORIGIN_TRANSLATOR_H

#include <QObject>
#include <QMap>
#include "eatranslator.h"

#include "services/plugin/PluginAPI.h"

class ORIGIN_PLUGIN_API OriginTranslator : public EATranslator
{
    Q_OBJECT
		
public:
    OriginTranslator();
    ~OriginTranslator();

    /**
    Add (or replace) a token
    */		
    void addToken(const QString& sToken, const QString& sValue);

    /**
    Remove an existing token
    */
    void removeToken(const QString& sToken);
	
    /**
    Override
    */
    virtual QString translate(const char *context, const char *sourceText, const char *disambiguation = 0, int n = -1) const;

private:
    QMap<QString, QString> mTokenMap;
};

#endif // ACTIVATIONTRANSLATOR_H
