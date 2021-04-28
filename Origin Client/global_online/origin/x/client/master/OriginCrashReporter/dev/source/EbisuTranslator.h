// Copyright (c) Electronic Arts, Inc. -- All Rights Reserved.

#ifndef ACTIVATIONTRANSLATOR_H
#define ACTIVATIONTRANSLATOR_H

#include <QObject>
#include <QMap>
#include "eatranslator.h"

class EbisuTranslator : public EATranslator
{
    Q_OBJECT
		
public:
	EbisuTranslator();
	~EbisuTranslator();

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
