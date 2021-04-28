#include <QDebug>

#include "WidgetPackageReader.h"

namespace WebWidget
{
    WidgetPackageReader::WidgetPackageReader(const WidgetPackage &package, const QLocale &locale) :
        mPackage(package), 
        mLocale(locale)
    {
        WidgetPackageFile configDoc = package.packageFile("config.xml");

        if (configDoc.exists())
        {
            // This order is important!

            // Parse our configuration
            mConfiguration = WidgetConfiguration(configDoc);
            // Build a list of locales to search for files in
            mSearchLocales = buildSearchLocales();
            // Find our start file
            mStartFile = findStartFile();
        }
        else
        {
            qWarning() << "Unable to locate widget config.xml";
        }
    }
    
    LocalizedWidgetFile WidgetPackageReader::findFile(const QString &widgetPath) const
    {
        return findFile(widgetPath, QByteArray(), QByteArray());
    }
    
    LocalizedWidgetFile WidgetPackageReader::findFile(const QString &widgetPath, const QByteArray &mediaType, const QByteArray &encoding) const
    {
        // Canonicalize the path first
        // We don't want eg ../fr/images/cats.png matching locales/fr/images/cats.png when looking up another locale
        QString canonicalWidgetPath(WidgetPackage::canonicalizePath(widgetPath));

        if (canonicalWidgetPath.isEmpty())
        {
            // Not a valid path, bail out early
            return LocalizedWidgetFile(widgetPath);
        }
        
        // Short circuit our lookup if the canonical path matches our start file
        // This allows us to return the correct media type and encoding
        if (canonicalWidgetPath == startFile().widgetPath())
        {
            return startFile();
        }

        // Do a locale based lookup
        for(QList<QString>::const_iterator it = mSearchLocales.constBegin();
            it != mSearchLocales.constEnd();
            it++)
        {
            QString localizedPath("locales/" + *it + "/" + canonicalWidgetPath);

            WidgetPackageFile localeOverridenFile(package().packageFile(localizedPath, mediaType, encoding));

            if (localeOverridenFile.exists())
            {
                return LocalizedWidgetFile::fromPackageFile(widgetPath, localeOverridenFile);
            }
        }

        // Fall back to looking at the root of the package
        WidgetPackageFile packageFile(package().packageFile(canonicalWidgetPath, mediaType, encoding));
        return LocalizedWidgetFile::fromPackageFile(widgetPath, packageFile);
    }

    QStringList WidgetPackageReader::buildSearchLocales() const
    {
        // Qt5 changed how bcp47Name() works. It only returns the sub id if they believe it's not needed.
        // https://bugreports.qt-project.org/browse/QTBUG-34872
        // However, we rely on the sub string for some locales e.g. ko_KR & ja_JP. Therefore, we have to cut
        // up the uiLanguages and hope that it's giving up the correct locale. Maybe in a future patch of Qt this will be fixed.
        QString bcp47Code;
        foreach(QString s, mLocale.uiLanguages())
        {
            if(s.count('-') == 1)
            {
                bcp47Code = s.toLower();
                break;
            }
        }
        QStringList wantedLocales;
        if(bcp47Code.isEmpty())
            qWarning() << "Dash-separated language (script and country) not found";
        else
        {
            // Make a list of preferred locales in descending order
            // We take the requested locale and use increasingly unspecific versions by removing subtags right to left
            wantedLocales << bcp47Code;
            wantedLocales << bcp47Code.split("-")[0];
        }

        // Add the widget's default locale on the end if specified
        QString defaultLocale(configuration().defaultLocale());

        if (!defaultLocale.isNull() && !wantedLocales.contains(defaultLocale))
        {
            wantedLocales.append(defaultLocale);
        }

        return wantedLocales;
    }
        
    LocalizedWidgetFile WidgetPackageReader::findStartFile() const
    {
        // Is there a custom start file defined?
        CustomStartFile customStartFile(configuration().customStartFile());
        if (!customStartFile.isNull())
        {
            LocalizedWidgetFile widgetFile(findFile(
                        customStartFile.widgetPath(),
                        customStartFile.mediaType(),
                        customStartFile.encoding()));

            // Fall through to a normal search if the file doesn't exist
            if (widgetFile.exists())
            {
                return widgetFile;
            }
        }

        QStringList defaultStartFiles;
        
        // Defined here: http://www.w3.org/TR/widgets/#default-start-files-table
        defaultStartFiles.append("index.htm");
        defaultStartFiles.append("index.html");
        defaultStartFiles.append("index.svg");
        defaultStartFiles.append("index.xhtml");
        defaultStartFiles.append("index.xht");

        foreach(const QString &defaultStartFile, defaultStartFiles)
        {
            LocalizedWidgetFile widgetFile(findFile(defaultStartFile));

            if (widgetFile.exists())
            {
                return widgetFile;
            }
        }

        return LocalizedWidgetFile();
    }
        
    LocalizedWidgetFile WidgetPackageReader::startFile() const
    {
        return mStartFile;
    }
        
    bool WidgetPackageReader::isValidPackage() const
    {
        return mConfiguration.isValid() && mStartFile.exists();
    }
}
