// Copyright (c) Electronic Arts, Inc. -- All Rights Reserved.

#include "EbisuTranslator.h"
#include <QVariant>


EbisuTranslator::EbisuTranslator()
{

}
EbisuTranslator::~EbisuTranslator()
{

}

QString EbisuTranslator::translate(const char *context, const char *sourceText, const char *disambiguation, int n) const
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

void EbisuTranslator::addToken(const QString& sToken, const QString& sValue)
{
	mTokenMap[sToken] = sValue;
}

void EbisuTranslator::removeToken(const QString& sToken)
{
	mTokenMap.remove(sToken);
}
