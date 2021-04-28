///////////////////////////////////////////////////////////////////////////////
// eatranslator.h
//
// Copyright (c) 2009 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef EATRANSLATOR_H
#define EATRANSLATOR_H

#include <QObject>
#include <QTranslator>
#include <QHash>
#include <QByteArray>
#include <QString>

typedef QHash<QByteArray, QString> LocaleHash;

class EATranslator : public QTranslator
{
    Q_OBJECT

public:
    EATranslator(QObject *parent=0);
   ~EATranslator();

    virtual QString translate(const char *context, const char *sourceText, const char *disambiguation = 0, int n = -1) const;
    bool loadHAL(const QString &filename, const QString &locale, const QString &suffix = ".xml");

    // Debug functionality.
    // This functionality modifies strings in ways which aren't useful for shipping but which serve to test the UI.
    void setQADebugLang(float width);  // Causes the addition or random characters or removal of characters. This is useful for testing German in particular. An amount of 1.0, means no width change. An amount of 1.2 means make strings 20% longer, 0.7 means make strings 30% shorter.
    float getQADebugLang() { return mQADebugWidth; }

    // Allows
#if defined(EA_DEBUG)
    static void debugShowKey(const bool showKey);
#endif

protected:
    bool parseXml(const QString &filename, const QString &locale, const QString &suffix, LocaleHash &mapLocale);

private:
    void duplicateKey(const char *keyCurrent, const char *keyNew);

protected:
    LocaleHash mCurrentLocaleHash;
    QString mCurrentLocale;

    float mQADebugWidth;     // Defaults to 1.0 (no change)

#if defined(EA_DEBUG)
    static bool sDebugShowKey;
#endif
};

#endif // EATRANSLATOR_H
