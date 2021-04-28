///////////////////////////////////////////////////////////////////////////////
// eatranslator.cpp
//
// Copyright (c) 2009 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include <QDebug>
#include <QFile>
#include <QXmlStreamReader>

#include "eatranslator.h"

#if defined(EA_DEBUG)
bool EATranslator::sDebugShowKey = false;
#endif

EATranslator::EATranslator(QObject *parent)
  : QTranslator(parent)
  , mCurrentLocaleHash()
  , mCurrentLocale()            // Defaults to empty (no locale)
  , mQADebugWidth(1.f)
{
}

EATranslator::~EATranslator()
{
}

// HAL file format sample

//<locStrings locale="es_ES">
//  <locString Key="access_application_name">Access es_ES</locString>
//  <locString Key="email">Correo electrónico</locString>
//  <locString Key="error_occurred">We're sorry, an error has occurred.</locString>
//  <locString Key="forgot_password">¿Olvidaste la contraseña?</locString>
//  <locString Key="log_in">Conectar</locString>
//</locStrings>
bool EATranslator::loadHAL(const QString &filename, const QString &locale, const QString &suffix)
{
    bool bLoadOk = false;
    const QString defaultLocale = "en_US";

    if (mCurrentLocale != locale)
    {
        // Load a different locale -> clear the maps
        mCurrentLocaleHash.clear();
    }

    // Avoid double parsing the locale for the default locale
    if (locale != defaultLocale)
    {
        // Always load default language first
        parseXml(filename, defaultLocale, suffix, mCurrentLocaleHash);
    }

    // Read the specfic locale on top of the default language
    bLoadOk = parseXml(filename, locale, suffix, mCurrentLocaleHash);
    if (bLoadOk)
    {
        // This locale was loaded OK, mark it!
        mCurrentLocale = locale;

#if defined(ORIGIN_MAC)
        // overrides for OS X
        duplicateKey("ebisu_mainmenuitem_services",             "Services");
        duplicateKey("ebisu_mainmenuitem_hide_app",             "Hide %1");
        duplicateKey("ebisu_mainmenuitem_hide_others",          "Hide Others");
        duplicateKey("ebisu_mainmenuitem_show_all",             "Show All");
        duplicateKey("ebisu_client_preferences",                "Preferences...");
        duplicateKey("ebisu_mainmenuitem_quit_app",             "Quit %1");
        duplicateKey("ebisu_mainmenuitem_about_app",            "About %1");
#endif

        // Overrides for Copy, Cut, Paste, etc.
        // NOTE: These are pulled straight from QTextControl::createStandardContextMenu()
        duplicateKey("ebisu_client_menu_copy",                    "&Copy");
        duplicateKey("ebisu_client_menu_cut",                    "Cu&t");
        duplicateKey("ebisu_client_menu_paste",                    "&Paste");
        duplicateKey("ebisu_client_menu_undo",                    "&Undo");
        duplicateKey("ebisu_client_menu_redo",                    "&Redo");
        duplicateKey("ebisu_client_menu_delete",                "Delete");
        duplicateKey("ebisu_client_menu_select_all",            "Select All");
        duplicateKey("ebisu_client_menu_copy_link_location",    "Copy &Link Location");
        duplicateKey("ebisu_client_shortcut_ctrl",                "Ctrl");
        duplicateKey("ebisu_client_shortcut_shift",                "Shift");
        duplicateKey("ebisu_client_shortcut_alt",                "Alt");
        duplicateKey("ebisu_client_shortcut_meta",                "Meta");
        
        duplicateKey("ebisu_client_scroll_scroll_here",         "Scroll here");
        duplicateKey("ebisu_client_scroll_top",                 "Top");
        duplicateKey("ebisu_client_scroll_bottom",              "Bottom");
        duplicateKey("ebisu_client_scroll_page_down",           "Page down");
        duplicateKey("ebisu_client_scroll_page_up",             "Page up");
        duplicateKey("ebisu_client_scroll_scroll_down",         "Scroll down");
        duplicateKey("ebisu_client_scroll_scroll_up",           "Scroll up");

        duplicateKey("ebisu_client_month_january",                "January");
        duplicateKey("ebisu_client_month_february",                "February");
        duplicateKey("ebisu_client_month_march",                "March");
        duplicateKey("ebisu_client_month_april",                "April");
        duplicateKey("ebisu_client_month_may",                    "May");
        duplicateKey("ebisu_client_month_june",                    "June");
        duplicateKey("ebisu_client_month_july",                    "July");
        duplicateKey("ebisu_client_month_august",                "August");
        duplicateKey("ebisu_client_month_september",            "September");
        duplicateKey("ebisu_client_month_october",                "October");
        duplicateKey("ebisu_client_month_november",                "November");
        duplicateKey("ebisu_client_month_december",                "December");
    }

    return bLoadOk;
}

bool EATranslator::parseXml(const QString &filename, const QString &locale, const QString &suffix, LocaleHash &mapLocale)
{
    QString realname = filename + locale + (suffix.isNull() ? QString::fromLatin1(".xml") : suffix);
    QFile file(realname);

    bool bSuccess = file.open(QFile::ReadOnly);

    if ( bSuccess )
    {
        QXmlStreamReader xml(&file);
        QString key;

        while (!xml.atEnd())
        {
            xml.readNext();
            if (xml.isStartElement())
            {
                if (xml.name() == "locStrings")
                {
                    if (xml.attributes().value("locale").toString() != locale)
                    {
                        bSuccess = false;
                        break;
                    }
                }

                else if (xml.name() == "locString")
                {
                    key = xml.attributes().value("Key").toString();
                    mapLocale[key.toLatin1()] = xml.readElementText();
                }
            }
        }
    }
    else
    {
        qDebug() << "Failed to open file" << file.fileName() << ":" << file.errorString();
    }

    return bSuccess;
}

// duplicateKey()
// Allows us to duplicate locale keys in case we want to override default behavior.
// IE. context menus display using tr("&Copy"), let's just make a duplicate of what
// we already have:  
//     mCurrentLocaleHash["&amp;Copy"] = mCurrentLocaleHash["ebisu_client_menu_copy"]
void EATranslator::duplicateKey(const char *keyCurrent, const char *keyNew)
{
    // Bail if we already have the new one
    if (mCurrentLocaleHash.find(keyNew) != mCurrentLocaleHash.end())
        return;

    LocaleHash::iterator iter = mCurrentLocaleHash.find(keyCurrent);

    if (iter != mCurrentLocaleHash.end())
    {
        mCurrentLocaleHash[keyNew] = mCurrentLocaleHash[keyCurrent];
    }
}

QString EATranslator::translate(const char* /* context */, const char *sourceText, const char* /* disambiguation */, int /* n */) const
{
#if defined(EA_DEBUG)
    if(sDebugShowKey)
        return QString(sourceText).prepend("*");
#endif

    // HAL files don't hold context, don't pay attention to that (left compatibility with Qt)
    QString translation;

    // Look for source text in current locale
    translation = mCurrentLocaleHash[sourceText];

    if (translation.isEmpty())
    {
        // Source text couldn't be found in default locale either
        // If requested, show source key (mostly for debugging purposes)
#ifdef EATRANSLATOR_SHOW_KEY
        translation = sourceText;
#else
        translation.clear();
#endif
}

    if(mQADebugWidth != 1.f)
    {
        const int currentSize  = translation.length();
        const int newSize      = (int)((currentSize * mQADebugWidth) + 0.5f);
        const int newSizeFixed = (newSize == 0) ? 1 : newSize; // Make sure there is at least one character.

        if(newSizeFixed < currentSize)
            translation.resize(newSizeFixed); // Just strip characters from the end
        else
        {
            int nCharsNeeded = newSizeFixed - currentSize + 2; // add 2 chars to make it a reasonable length
            if (nCharsNeeded > 1)
            {
                translation.append((QChar)' ');
                nCharsNeeded--;
            }

            for (int nCount=0; nCount < nCharsNeeded; ++nCount)
            {
                const QChar fill[5] = { 0x0E10, 0x042F, 0x0E42, 0x0416, 0x0E28};
                translation.append((QChar)fill[nCount % 5]);
            }
        }
    }

    return translation;
}

void EATranslator::setQADebugLang(float amount)
{
    mQADebugWidth = amount;
}

#if defined(EA_DEBUG)
void EATranslator::debugShowKey(const bool showKey)
{
    sDebugShowKey = showKey;
}
#endif

